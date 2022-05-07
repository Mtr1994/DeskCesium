let context;
let cesiumViewer;
let measureStatus = 0;

// 初始化
function init() {
	if (typeof qt != 'undefined')
    {
        new QWebChannel(qt.webChannelTransport, function(channel) { 
			context = channel.objects.context; 
			
			context.sgl_send_client_msg.connect(function(action, type, arg) {
				if (action === "add") {
					if (type === "kml") {
						addKmlEntity(arg);
					} else if (type === "kmz") {
						addKmzEntity(arg)
					} else if (type === "tif") {
						addTifEntity(arg)
					}
				} else if (action === "changesource") {
					changeDataSourceVisible(type, arg);
				} else if (action === "changetif") {
					let entity = cesiumViewer.entities.getById(type)
					if (undefined === entity) return;
					entity.show = (arg === "true") ? true : false;
				} else if (action === "changeMLA") {
					changeMeasureLineSourceVisible(type, arg);
				} else if (action === "changeMPA") {
					changeMeasurePolygnSourceVisible(type, arg);
				} else if (action === "delete") {
					if (type === "tif") {
						let entity = cesiumViewer.entities.getById(arg)
						if (undefined === entity) return;
						cesiumViewer.entities.remove(entity);
						context.recvMsg("delete", type, true, arg);
					} else if (type == "MLA") {
						deleteMeasureLineSource(arg);
						context.recvMsg("delete", type, true, arg);
					} else if (type == "MPA") {
						deleteMeasurePolygnSource(arg);
						context.recvMsg("delete", type, true, arg);
					} else {
						deleteDataSource(type, arg)
					}
				} else if (action === "measure") {
					if (type == "MLA") startMeasureLine(arg);
					else if (type == "MPA")  startMeasurePolygn(arg);
					else context.recvMsg("measure", type, false, arg);
				}
			});
		});
    }
    else
    {
        alert("qt对象获取失败！");
    }

	cesiumViewer = new Cesium.Viewer('cesiumContainer', {
	  shadows: false,
	  timeline: false,
	  baseLayerPicker: false,
	  fullscreenButton: false,
	  selectionIndicator: false,
	  homeButton: false,
	  animation: false,
	  infoBox: false,
	  geocoder: false,
	  navigationHelpButton: false,
	  imageryProvider: new Cesium.WebMapServiceImageryProvider({
		url: 'https://www.gmrt.org/services/mapserver/wms_merc',
		layers: 'GMRT'
	  })
	})
	cesiumViewer._cesiumWidget._creditContainer.style.display = 'none'
	cesiumViewer.scene.sun.show = false
	cesiumViewer.scene.moon.show = false
	cesiumViewer.scene.fog.show = false
	cesiumViewer.scene.skyAtmosphere.show = false
	cesiumViewer.scene.sun.show = false
	cesiumViewer.scene.skyBox.show = false

	cesiumViewer.cesiumWidget.screenSpaceEventHandler.removeInputAction(Cesium.ScreenSpaceEventType.LEFT_DOUBLE_CLICK)
}

function sendMessage(msg, arg) {
	if(typeof context == 'undefined')
    {
        alert("context对象获取失败！");
    }
    else
    {
        context.slot_recv_html_meg(msg, arg);
    }
}

function handleDrop(event) {
	event.preventDefault();
}

function handleDragOver(event) {
	event.preventDefault();
}

// 添加 kml 实体
function addKmlEntity(path) {
	const options = { camera: cesiumViewer.scene.camera, canvas: cesiumViewer.scene.canvas, screenOverlayContainer: cesiumViewer.container, clampToGround: true };
	cesiumViewer.dataSources.add(Cesium.KmlDataSource.load(path, options)).then(function (dataSource) { 
		let entitySize = dataSource.entities.values.length;
		for (let i = 0; i < entitySize; i++) {
			dataSource.entities.values[i].show = true;
		}
		context.recvMsg("add", "source", true, dataSource.name);
	});
}
// 添加 kmz 实体
function addKmzEntity(path) {
	const options = { camera: cesiumViewer.scene.camera, canvas: cesiumViewer.scene.canvas, screenOverlayContainer: cesiumViewer.container};
	cesiumViewer.dataSources.add(Cesium.KmlDataSource.load(path, options)).then(function (dataSource) {
		let entitySize = dataSource.entities.values.length;
		for (let i = 0; i < entitySize; i++) {
			dataSource.entities.values[i].show = true;
		}
		context.recvMsg("add", "source", true, dataSource.name);
	});
}
// 添加 tif 实体
function addTifEntity(arg) {
	let array = arg.split(",")
	if (array.length !== 5) {
		context.recvMsg("add", "tif",false, arg);
	} else {
		let size = cesiumViewer.entities.values.length;
		cesiumViewer.entities.add({
			id: array[4],
			rectangle: {
			coordinates: Cesium.Rectangle.fromDegrees(parseFloat(array[0]), parseFloat(array[1]), parseFloat(array[2]), parseFloat(array[3])),
			material: array[4],
			zIndex: size,
		  }
		})
		
		context.recvMsg("add", "tif", true, array[4]);
	}
}

// 修改数据集可视化状态
function changeDataSourceVisible(name, visible) {
	let dataSourceArray = cesiumViewer.dataSources.getByName(name);
	if (undefined == dataSourceArray) return;
	
	let len = dataSourceArray.length;
	for (let i = 0; i < len; i++) {
		dataSourceArray[i].show = (visible === "true") ? true : false;
		
		let screenOverlaySize = dataSourceArray[i]._screenOverlays.length;
		for (let j = 0; j < screenOverlaySize; j++) {
			dataSourceArray[i]._screenOverlays[j].style.visibility = dataSourceArray[i].show ? "visible" : "hidden";
		}
	}
}

// 删除数据集
function deleteDataSource(type, name) {
	let dataSourceArray = cesiumViewer.dataSources.getByName(name);
	if (undefined == dataSourceArray) return;
	
	let len = dataSourceArray.length;
	for (let i = 0; i < len; i++) {
		cesiumViewer.dataSources.remove(dataSourceArray[i], true);
	}
	context.recvMsg("delete", type, true, name);
}

// 距离测量 (arg 为 时间 id)
function startMeasureLine(arg) {
	if (measureStatus == 1) return;
	measureStatus = 1;
    var handler = new Cesium.ScreenSpaceEventHandler(cesiumViewer.scene._imageryLayerCollection);
    var positions = [];
    var poly = null;
    var distance = 0;
    var cartesian = null;
    var floatingPoint;
    var labelPt;
    handler.setInputAction(function (movement) {
        let ray = cesiumViewer.camera.getPickRay(movement.endPosition);
        cartesian = cesiumViewer.scene.globe.pick(ray, cesiumViewer.scene);
        if (!Cesium.defined(cartesian)) //跳出地球时异常
            return;
        if (positions.length >= 2) {
            if (!Cesium.defined(poly)) {
                poly = new PolyLinePrimitive(positions);
            } else {
                positions.pop();
                positions.push(cartesian);
            }
        }
    }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);
 
    handler.setInputAction(function (movement) {
        let ray = cesiumViewer.camera.getPickRay(movement.position);
        cartesian = cesiumViewer.scene.globe.pick(ray, cesiumViewer.scene);
        if (!Cesium.defined(cartesian)) //跳出地球时异常
            return;
        if (positions.length == 0) {
            positions.push(cartesian.clone());
        }
        positions.push(cartesian);
        //记录鼠标单击时的节点位置，异步计算贴地距离
        labelPt = positions[positions.length - 1];
        if (positions.length > 2) {
            getSpaceDistance(positions);
			if (positions.length == 3) {
				// 返回测线 id
				context.recvMsg("add", "MLA", true, arg);
			}
        } else if (positions.length == 2) {
            //在三维场景中添加Label
            floatingPoint = cesiumViewer.entities.add({
				id: "MLA" + arg + cesiumViewer.entities.values.length,
                name: '空间距离',
                position: labelPt,
                point: {
                    pixelSize: 5,
                    color: Cesium.Color.RED,
                    outlineColor: Cesium.Color.WHITE,
                    outlineWidth: 2,
                }
            });
        }
    }, Cesium.ScreenSpaceEventType.LEFT_CLICK);
 
    handler.setInputAction(function (movement) {
        handler.destroy(); //关闭事件句柄
        handler = undefined;
        positions.pop(); //最后一个点无效
        if (positions.length == 1)
            cesiumViewer.entities.remove(floatingPoint);
		
		measureStatus = 0;	
    }, Cesium.ScreenSpaceEventType.RIGHT_CLICK);
 
    var PolyLinePrimitive = (function () {
        function _(positions) {
            this.options = {
				id: "MLA" + arg + cesiumViewer.entities.values.length,
                name: '直线',
                polyline: {
                    show: true,
                    positions: [],
                    material: Cesium.Color.CHARTREUSE,
                    width: 3,
                    clampToGround: true
                }
            };
            this.positions = positions;
            this._init();
        }
 
        _.prototype._init = function () {
            var _self = this;
            var _update = function () {
                return _self.positions;
            };
            //实时更新polyline.positions
            this.options.polyline.positions = new Cesium.CallbackProperty(_update, false);
            var addedEntity = cesiumViewer.entities.add(this.options);
        };
 
        return _;
    })();
 
    //空间两点距离计算函数
    function getSpaceDistance(positions) {
        //只计算最后一截，与前面累加
        //因move和鼠标左击事件，最后两个点坐标重复
        var i = positions.length - 3;
        var point1cartographic = Cesium.Cartographic.fromCartesian(positions[i]);
        var point2cartographic = Cesium.Cartographic.fromCartesian(positions[i + 1]);
        getTerrainDistance(point1cartographic, point2cartographic);
    }
 
    function getTerrainDistance(point1cartographic, point2cartographic) {
        var geodesic = new Cesium.EllipsoidGeodesic();
        geodesic.setEndPoints(point1cartographic, point2cartographic);
        var s = geodesic.surfaceDistance;
        var cartoPts = [point1cartographic];
        for (var jj = 1000; jj < s; jj += 1000) {　　//分段采样计算距离
            var cartoPt = geodesic.interpolateUsingSurfaceDistance(jj);
            cartoPts.push(cartoPt);
        }
        cartoPts.push(point2cartographic);
        //返回两点之间的距离
        Cesium.sampleTerrain(cesiumViewer.terrainProvider, 8, cartoPts).then(function (updatedPositions) {
            for (var jj = 0; jj < updatedPositions.length - 1; jj++) {
                var geoD = new Cesium.EllipsoidGeodesic();
                geoD.setEndPoints(updatedPositions[jj], updatedPositions[jj + 1]);
                var innerS = geoD.surfaceDistance;
                innerS = Math.sqrt(Math.pow(innerS, 2) + Math.pow(updatedPositions[jj + 1].height - updatedPositions[jj].height, 2));
                distance += innerS;
            }
            //在三维场景中添加Label
            var lon1 = cesiumViewer.scene.globe.ellipsoid.cartesianToCartographic(labelPt).longitude;
            var lat1 = cesiumViewer.scene.globe.ellipsoid.cartesianToCartographic(labelPt).latitude;
            var textDisance = distance.toFixed(2) + "米";
            if (distance > 10000)
                textDisance = (distance / 1000.0).toFixed(2) + "千米";
            floatingPoint = cesiumViewer.entities.add({
				id: "MLA" + arg + cesiumViewer.entities.values.length,
                name: '贴地距离',
                position: labelPt,
                point: {
                    pixelSize: 5,
                    color: Cesium.Color.RED,
                    outlineColor: Cesium.Color.WHITE,
                    outlineWidth: 2,
                },
                label: {
                    text: textDisance,
                    font: '18px sans-serif',
                    fillColor: Cesium.Color.GOLD,
                    style: Cesium.LabelStyle.FILL_AND_OUTLINE,
                    outlineWidth: 2,
                    verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
                    pixelOffset: new Cesium.Cartesian2(20, -20),
                }
            });
        });
    }
}

// 修改测量线可视状态
function changeMeasureLineSourceVisible(id, visible) {
	let len = cesiumViewer.entities.values.length
	for (let i = 0 ; i <= len; i++) {
		let entity = cesiumViewer.entities.getById("MLA" + id + i);
		if (undefined === entity) continue;
		entity.show = (visible === "true") ? true : false;
	}
}

// 删除测量线
function deleteMeasureLineSource(id) {
	let entityIdArray = []
	let len = cesiumViewer.entities.values.length
	for (let i = 0 ; i < len; i++) {
		let entity = cesiumViewer.entities.values[i]
		if (undefined === entity) continue;
		let entityId = entity.id;
		
		if (entityId.startsWith("MLA" + id)) {
			entityIdArray.push(entityId)
		}
	}
	
	len = entityIdArray.length;
	for (let j = 0; j < len; j++) {
		cesiumViewer.entities.removeById(entityIdArray[j])
	}
}

//面积测量 (arg 为 时间 id)
function startMeasurePolygn(arg) {
	if (measureStatus == 1) return;
	measureStatus = 1;
    // 鼠标事件
    var handler = new Cesium.ScreenSpaceEventHandler(cesiumViewer.scene._imageryLayerCollection);
    var positions = [];
    var tempPoints = [];
    var polygon = null;
    var cartesian = null;
    var floatingPoint;//浮动点
    handler.setInputAction(function (movement) {
        let ray = cesiumViewer.camera.getPickRay(movement.endPosition);
        cartesian = cesiumViewer.scene.globe.pick(ray, cesiumViewer.scene);
		if (!Cesium.defined(cartesian)) //跳出地球时异常
            return;
        positions.pop();//移除最后一个
        positions.push(cartesian);
        if (positions.length >= 2) {
            var dynamicPositions = new Cesium.CallbackProperty(function () {
                return new Cesium.PolygonHierarchy(positions);
                return positions;
            }, false);
            polygon = PolygonPrimitive(dynamicPositions);
        }
    }, Cesium.ScreenSpaceEventType.MOUSE_MOVE);
 
    handler.setInputAction(function (movement) {
        let ray = cesiumViewer.camera.getPickRay(movement.position);
        cartesian = cesiumViewer.scene.globe.pick(ray, cesiumViewer.scene);
        if (positions.length == 0) {
            positions.push(cartesian.clone());
        }
        positions.push(cartesian);
		
		// 返回测量区域 id
		if (positions.length === 2) context.recvMsg("add", "MPA", true, arg);
		
        //在三维场景中添加点
        var cartographic = Cesium.Cartographic.fromCartesian(positions[positions.length - 1]);
        var longitudeString = Cesium.Math.toDegrees(cartographic.longitude);
        var latitudeString = Cesium.Math.toDegrees(cartographic.latitude);
        var heightString = cartographic.height;
        var labelText = "(" + longitudeString.toFixed(2) + "," + latitudeString.toFixed(2) + ")";
        tempPoints.push({ lon: longitudeString, lat: latitudeString, hei: heightString });
        floatingPoint = cesiumViewer.entities.add({
			id: "MPA" + arg + cesiumViewer.entities.values.length,
            name: '多边形面积',
            position: positions[positions.length - 1],
            point: {
                pixelSize: 5,
                color: Cesium.Color.RED,
                outlineColor: Cesium.Color.WHITE,
                outlineWidth: 2,
                heightReference: Cesium.HeightReference.CLAMP_TO_GROUND
            }
        });
    }, Cesium.ScreenSpaceEventType.LEFT_CLICK);
    handler.setInputAction(function (movement) {
        handler.destroy();
        positions.pop();
        var textArea = getArea(tempPoints) + "平方公里";
        cesiumViewer.entities.add({
			id: "MPA" + arg + cesiumViewer.entities.values.length,
            name: '多边形面积',
            position: positions[positions.length - 1],
            label: {
                text: textArea,
                font: '18px sans-serif',
                fillColor: Cesium.Color.GOLD,
                style: Cesium.LabelStyle.FILL_AND_OUTLINE,
                outlineWidth: 2,
                verticalOrigin: Cesium.VerticalOrigin.BOTTOM,
                pixelOffset: new Cesium.Cartesian2(20, -40),
                heightReference: Cesium.HeightReference.CLAMP_TO_GROUND
            }
        });
		
		measureStatus = 0;
    }, Cesium.ScreenSpaceEventType.RIGHT_CLICK);
    var radiansPerDegree = Math.PI / 180.0;//角度转化为弧度(rad)
    var degreesPerRadian = 180.0 / Math.PI;//弧度转化为角度
    //计算多边形面积
    function getArea(points) {
        var res = 0;
        //拆分三角曲面
        for (var i = 0; i < points.length - 2; i++) {
            var j = (i + 1) % points.length;
            var k = (i + 2) % points.length;
            var totalAngle = Angle(points[i], points[j], points[k]);
            var dis_temp1 = distance(positions[i], positions[j]);
            var dis_temp2 = distance(positions[j], positions[k]);
            res += dis_temp1 * dis_temp2 * Math.abs(Math.sin(totalAngle));
        }
        return (res / 1000000.0).toFixed(4);
    }
 
    /*角度*/
    function Angle(p1, p2, p3) {
        var bearing21 = Bearing(p2, p1);
        var bearing23 = Bearing(p2, p3);
        var angle = bearing21 - bearing23;
        if (angle < 0) {
            angle += 360;
        }
        return angle;
    }
    /*方向*/
    function Bearing(from, to) {
        var lat1 = from.lat * radiansPerDegree;
        var lon1 = from.lon * radiansPerDegree;
        var lat2 = to.lat * radiansPerDegree;
        var lon2 = to.lon * radiansPerDegree;
        var angle = -Math.atan2(Math.sin(lon1 - lon2) * Math.cos(lat2), Math.cos(lat1) * Math.sin(lat2) - Math.sin(lat1) * Math.cos(lat2) * Math.cos(lon1 - lon2));
        if (angle < 0) {
            angle += Math.PI * 2.0;
        }
        angle = angle * degreesPerRadian;//角度
        return angle;
    }
 
    function PolygonPrimitive(positions) {
        polygon = cesiumViewer.entities.add({
			id: "MPA" + arg + cesiumViewer.entities.values.length,
            polygon: {
                hierarchy: positions,
                material: Cesium.Color.GREEN.withAlpha(0.1),
            }
        });
 
    }
 
    function distance(point1, point2) {
        var point1cartographic = Cesium.Cartographic.fromCartesian(point1);
        var point2cartographic = Cesium.Cartographic.fromCartesian(point2);
        /**根据经纬度计算出距离**/
        var geodesic = new Cesium.EllipsoidGeodesic();
        geodesic.setEndPoints(point1cartographic, point2cartographic);
        var s = geodesic.surfaceDistance;
        //返回两点之间的距离
        s = Math.sqrt(Math.pow(s, 2) + Math.pow(point2cartographic.height - point1cartographic.height, 2));
        return s;
    }
}

// 修改测量面积可视状态
function changeMeasurePolygnSourceVisible(id, visible) {
	let len = cesiumViewer.entities.values.length
	for (let i = 0 ; i <= len; i++) {
		let entity = cesiumViewer.entities.getById("MPA" + id + i);
		if (undefined === entity) continue;
		entity.show = (visible === "true") ? true : false;
	}
}

// 删除测量区域
function deleteMeasurePolygnSource(id) {
	let entityIdArray = []
	let len = cesiumViewer.entities.values.length
	for (let i = 0 ; i < len; i++) {
		let entity = cesiumViewer.entities.values[i]
		if (undefined === entity) continue;
		let entityId = entity.id;
		
		if (entityId.startsWith("MPA" + id)) {
			entityIdArray.push(entityId)
		}
	}
	
	len = entityIdArray.length;
	for (let j = 0; j < len; j++) {
		cesiumViewer.entities.removeById(entityIdArray[j])
	}
}
