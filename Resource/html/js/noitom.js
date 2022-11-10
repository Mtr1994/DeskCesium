let context;
let cesiumViewer;
let measureStatus = 0;
let glMouseOverHandler = undefined;
let hoverLongitude = 0.0;
let hoverLatitude = 0.0;
let createMeasurePoint = false

// 初始化
function init() {
	if (typeof qt != 'undefined')
    {
        new QWebChannel(qt.webChannelTransport, function(channel) { 
			context = channel.objects.context; 
			context.sgl_add_entity.connect(function(type, path) {
				if (type === "kml") {
					addKmlEntity(path);
				} else if (type === "kmz") {
					addKmzEntity(path)
				} else if (type === "tif") {
					addTifEntity(path)
				} else if (type === "grd") {
					addGrdEntity(path)
				}
			});
			context.sgl_change_entity_visible.connect(function(type, id, visible, parentId) {
				if (type === "kml" || type === "kmz") {
					if (parentId !== "") {
						changeDataSourceChildVisible(id, visible, parentId)
					} else {
						changeDataSourceVisible(id, visible, parentId);
					}
				} else if (type === "mla") {
					changeMeasureLineSourceVisible(id, visible);
				} else if (type === "mpa") {
					changeMeasurePolygnSourceVisible(id, visible);
				} else {
					let entity = cesiumViewer.entities.getById(id)
					if (undefined === entity) return;
					entity.show = visible;
				}
			});
			context.sgl_delete_entity.connect(function(type, id) {
				if (type === "kml" || type === "kmz") {
					deleteDataSource(type, id)
				} else if (type == "mla") {
					deleteMeasureLineSource(id);
				} else if (type == "mpa") {
					deleteMeasurePolygnSource(id);
				} else {
					let entity = cesiumViewer.entities.getById(id)
					cesiumViewer.entities.remove(entity);
				}
				context.recvMsg("delete", type, true, id);
			});
			context.sgl_start_measure.connect(function(type, id) {
				if (type == "mla") {
					startMeasureLine(id);
				} else if (type == "mpa")  {
					startMeasurePolygn(id);
				}
			});
			context.sgl_fly_to_entity.connect(function(type, id, parentId) {
				if (parentId !== "") {
					let dataSourceArray = cesiumViewer.dataSources.getByName(parentId);
					if (undefined == dataSourceArray) return;
					let len = dataSourceArray.length;
					for (let i = 0; i < len; i++) {
						let entity = dataSourceArray[i].entities.getById(id)
						if (undefined === entity) continue;
						cesiumViewer.flyTo(entity, {duration: 1, maximumHeight: 15000, offset: new Cesium.HeadingPitchRange(0, -90, 0.0)});
					}
				} else {
					let entity = cesiumViewer.entities.getById(id);
					if (undefined == entity) return;
					cesiumViewer.flyTo(entity, {duration: 1, maximumHeight: 15000, offset: new Cesium.HeadingPitchRange(0, -90, 0.0)});
				}
			});
			context.sgl_change_mouse_over_status.connect(function(isOpen) {
				if (isOpen) {
					openMousePicking();
				} else {
					closeMousePicking();
				}
			});
			context.sgl_search_mouse_over_altitude.connect(function(longitude, latitude, result, altitude) {
				if (hoverLongitude !== parseFloat(longitude).toFixed(6) || hoverLatitude !== parseFloat(latitude).toFixed(6)) return;
				if (result) {
					let geoinfo = document.getElementById("geoinfo");
					geoinfo.innerHTML = `Longitude: ${`${hoverLongitude}`.slice(-12)}\u00B0` + `&nbsp&nbsp&nbsp&nbsp Latitude: ${`${hoverLatitude}`.slice(-12)}\u00B0` + `&nbsp&nbsp&nbsp&nbspElevation: ${`${altitude.toFixed(0)}`.slice(-12)} m (Local)`;
				} else {
					if (!navigator.onLine) {
						geoinfo.innerHTML = `Longitude: ${`${hoverLongitude}`.slice(-12)}\u00B0` + `&nbsp&nbsp&nbsp&nbsp Latitude: ${`${hoverLatitude}`.slice(-12)}\u00B0` + `&nbsp&nbsp&nbsp&nbspElevation: nds`;
					} else {
						let xmlHttp = new XMLHttpRequest();
						let url = `https://www.gmrt.org/services/PointServer?latitude=${hoverLatitude}&longitude=${hoverLongitude}&&format=json`;
						xmlHttp.onreadystatechange = function () {
							if (xmlHttp.readyState == 4 && xmlHttp.status == 200){
								let response_data = xmlHttp.response;
								let jsonData = JSON.parse(response_data);
								if (hoverLongitude == parseFloat(jsonData.longitude).toFixed(6) && hoverLatitude == parseFloat(jsonData.latitude).toFixed(6)) {
									geoinfo.innerHTML = `Longitude: ${`${hoverLongitude}`.slice(-12)}\u00B0` + `&nbsp&nbsp&nbsp&nbsp Latitude: ${`${hoverLatitude}`.slice(-12)}\u00B0` + `&nbsp&nbsp&nbsp&nbspElevation: ${`${parseFloat(jsonData.elevation).toFixed(0)}`.slice(-12)} m (Remote)`;
								}
							}
						};
						xmlHttp.open('GET', url, true);
						xmlHttp.send();
					}
				}
			});
			
			context.recvMsg("init", "", true, "");
		});
    }
    else
    {
        alert("qt对象获取失败！");
    }
	
	let earthMap;
	if (navigator.onLine) {
		earthMap = new Cesium.WebMapServiceImageryProvider({ url: 'https://www.gmrt.org/services/mapserver/wms_merc?', layers: 'GMRT'});
	} else {
		earthMap = new Cesium.TileMapServiceImageryProvider({ url: Cesium.buildModuleUrl("Assets/Textures/NaturalEarthII")});
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
	  imageryProvider: earthMap
	})
	cesiumViewer._cesiumWidget._creditContainer.style.display = 'none'
	cesiumViewer.scene.sun.show = false
	cesiumViewer.scene.moon.show = false
	cesiumViewer.scene.fog.show = false
	cesiumViewer.scene.skyAtmosphere.show = false
	cesiumViewer.scene.sun.show = false
	cesiumViewer.scene.skyBox.show = false
	
	// 曲线抗锯齿处理
	cesiumViewer.scene.fxaa = false;
	cesiumViewer.scene.postProcessStages.fxaa.enabled = true;
	if (Cesium.FeatureDetection.supportsImageRenderingPixelated()) { // 判断是否支持图像渲染像素化处理
		cesiumViewer.resolutionScale = window.devicePixelRatio;
	}

	// 禁止双击聚焦操作
	cesiumViewer.cesiumWidget.screenSpaceEventHandler.removeInputAction(Cesium.ScreenSpaceEventType.LEFT_DOUBLE_CLICK)
	
	var handler = new Cesium.ScreenSpaceEventHandler(cesiumViewer.scene.canvas);
	handler.setInputAction(function (movement) {
		var pick = cesiumViewer.scene.pick(movement.position);
		let entityDescription = document.getElementById("entityDescription");
		if (undefined === pick) {
			if (undefined !== entityDescription) {
				entityDescription.style.visibility = "hidden";
			}
		} else {
			if (undefined !== entityDescription) {
				console.log("pick.id._description", pick.id._description)
				if (undefined === pick.id._description) {
					entityDescription.style.visibility = "visible";
					// entityDescription.innerHTML = "啊哈哈哈哈"; //pick.id._description._value
				}
			}
		}
	}, Cesium.ScreenSpaceEventType.LEFT_CLICK);
}

function handleDrop(event) {
	event.preventDefault();
}

function handleDragOver(event) {
	event.preventDefault();
}

// 添加 kml 实体
function addKmlEntity(path) {
	const options = { camera: cesiumViewer.scene.camera, canvas: cesiumViewer.scene.canvas, screenOverlayContainer: cesiumViewer.container };
	cesiumViewer.dataSources.add(Cesium.KmlDataSource.load(path, options)).then(function (dataSource) { 
		let entitySize = dataSource.entities.values.length;
		let entityList = "";
		let screenOverlaySize = dataSource._screenOverlays.length;
		for (let j = 0; j < screenOverlaySize; j++) {
			entityList = entityList + "Legend," + j + "#";
		}
		
		for (let i = 0; i < entitySize; i++) {
			let entity = dataSource.entities.values[i]
			entity.show = true;
			entityList = entityList + entity.name + "," + entity.id + "#";
		}
		context.recvMsg("add", "kml", true, dataSource.name, entityList);
	});
}

// 添加 kmz 实体
function addKmzEntity(path) {
	const options = { camera: cesiumViewer.scene.camera, canvas: cesiumViewer.scene.canvas, screenOverlayContainer: cesiumViewer.container};
	cesiumViewer.dataSources.add(Cesium.KmlDataSource.load(path, options)).then(function (dataSource) {
		let entitySize = dataSource.entities.values.length;
		let entityList = "";
		let screenOverlaySize = dataSource._screenOverlays.length;
		for (let j = 0; j < screenOverlaySize; j++) {
			entityList = entityList + "Legend," + j + "#";
		}
		
		for (let i = 0; i < entitySize; i++) {
			let entity = dataSource.entities.values[i]
			entity.show = true;
			entityList = entityList + entity.name + "," + entity.id + "#";
		}
		context.recvMsg("add", "kmz", true, dataSource.name, entityList);
	});
}

// 添加 tif 实体
function addTifEntity(arg) {
	let array = arg.split(",")
	if (array.length !== 6) {
		context.recvMsg("add", "tif",false, arg);
	} else {
		
		let xhr = new XMLHttpRequest();
		xhr.open('GET', array[4]);
		xhr.responseType = 'arraybuffer';
		xhr.onload = function (e) {
			Tiff.initialize({TOTAL_MEMORY: parseInt(array[5]) * 2})
			let tiff = new Tiff({buffer: xhr.response});
			let canvas = tiff.toCanvas();
			let size = cesiumViewer.entities.values.length;
			cesiumViewer.entities.add({
			  id: array[4],
			  rectangle: {
				coordinates: Cesium.Rectangle.fromDegrees(parseFloat(array[0]), parseFloat(array[1]), parseFloat(array[2]), parseFloat(array[3])),
				material: canvas,
				zIndex: size,
			  }
			});
			
			context.recvMsg("add", "tif", true, array[4]);
		};
		xhr.send();
	}
}

// 添加 grd 实体
function addGrdEntity(arg) {
	let array = arg.split(",")
	if (array.length !== 5) {
		context.recvMsg("add", "grd",false, arg);
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
		
		context.recvMsg("add", "grd", true, array[4]);
	}
}


// 修改数据集可视化状态
function changeDataSourceVisible(name, visible) {
	let dataSourceArray = cesiumViewer.dataSources.getByName(name);
	if (undefined == dataSourceArray) return;
	
	let len = dataSourceArray.length;
	for (let i = 0; i < len; i++) {
		dataSourceArray[i].show = visible;
		
		let screenOverlaySize = dataSourceArray[i]._screenOverlays.length;
		for (let j = 0; j < screenOverlaySize; j++) {
			dataSourceArray[i]._screenOverlays[j].style.visibility = dataSourceArray[i].show ? "visible" : "hidden";
		}
	}
}

// 修改数据集子集可视化状态
function changeDataSourceChildVisible(id, visible, name) {
	let dataSourceArray = cesiumViewer.dataSources.getByName(name);
	if (undefined == dataSourceArray) return;
	
	if (id.startsWith("Legend")) {
		let array = id.split("-");
		
		if (array.length != 2) return;
		let index = parseInt(array[1]);
		let len = dataSourceArray.length;
		for (let i = 0; i < len; i++) {
			let screenOverlaySize = dataSourceArray[i]._screenOverlays.length;
			if (screenOverlaySize <= index) continue;
			for (let j = 0; j < screenOverlaySize; j++) {
				dataSourceArray[i]._screenOverlays[index].style.visibility = visible ? "visible" : "hidden";
			}
		}
	} else {
		let len = dataSourceArray.length;
		for (let i = 0; i < len; i++) {
			let entity = dataSourceArray[i].entities.getById(id);
			if (undefined == entity) continue;
			entity.show = visible;
			break;
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
}

// 距离测量 (arg 为 时间 id) （可能解决大量实体导致的卡顿问题 https://www.jianshu.com/p/5a74c607a591）
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
	
	createMeasurePoint = false;
	
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
		if (createMeasurePoint) return;
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
			createMeasurePoint = true;
            getSpaceDistance(positions);
			if (positions.length == 3) {
				// 返回测线 id
				context.recvMsg("add", "mla", true, arg);
			}
        } else if (positions.length == 2) {
            //在三维场景中添加Label
            floatingPoint = cesiumViewer.entities.add({
				id: "mla" + arg + cesiumViewer.entities.values.length,
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
				id: "mla" + arg + cesiumViewer.entities.values.length,
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
				id: "mla" + arg + cesiumViewer.entities.values.length,
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
			
			createMeasurePoint = false;
        });
    }
}

// 修改测量线可视状态
function changeMeasureLineSourceVisible(id, visible) {
	let len = cesiumViewer.entities.values.length
	for (let i = 0 ; i <= len; i++) {
		let entity = cesiumViewer.entities.getById("mla" + id + i);
		if (undefined === entity) continue;
		entity.show = visible;
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
		
		if (entityId.startsWith("mla" + id)) {
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
		if (positions.length === 2) context.recvMsg("add", "mpa", true, arg);
		
        //在三维场景中添加点
        var cartographic = Cesium.Cartographic.fromCartesian(positions[positions.length - 1]);
        var longitudeString = Cesium.Math.toDegrees(cartographic.longitude);
        var latitudeString = Cesium.Math.toDegrees(cartographic.latitude);
        var heightString = cartographic.height;
        var labelText = "(" + longitudeString.toFixed(2) + "," + latitudeString.toFixed(2) + ")";
        tempPoints.push({ lon: longitudeString, lat: latitudeString, hei: heightString });
        floatingPoint = cesiumViewer.entities.add({
			id: "mpa" + arg + cesiumViewer.entities.values.length,
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
			id: "mpa" + arg + cesiumViewer.entities.values.length,
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
			id: "mpa" + arg + cesiumViewer.entities.values.length,
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
		let entity = cesiumViewer.entities.getById("mpa" + id + i);
		if (undefined === entity) continue;
		entity.show = visible;
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
		
		if (entityId.startsWith("mpa" + id)) {
			entityIdArray.push(entityId)
		}
	}
	
	len = entityIdArray.length;
	for (let j = 0; j < len; j++) {
		cesiumViewer.entities.removeById(entityIdArray[j])
	}
}

// 开启鼠标浮动展示经纬度位置
function openMousePicking() {
	let geoinfo = document.getElementById("geoinfo");
	geoinfo.style.visibility = "visible";
	
	// Mouse over the globe to see the cartographic position
	glMouseOverHandler = new Cesium.ScreenSpaceEventHandler(cesiumViewer.scene.canvas);
	glMouseOverHandler.setInputAction(function (movement) {
	  const cartesian = cesiumViewer.camera.pickEllipsoid(
		movement.endPosition,
		cesiumViewer.scene.globe.ellipsoid
	  );
	  if (cartesian) {
		const cartographic = Cesium.Cartographic.fromCartesian(
		  cartesian
		);
		hoverLongitude = Cesium.Math.toDegrees(
		  cartographic.longitude
		).toFixed(6);
		hoverLatitude = Cesium.Math.toDegrees(
		  cartographic.latitude
		).toFixed(6);
		
		// geoinfo.innerHTML = `Longitude: ${`${hoverLongitude}`.slice(-12)}\u00B0` + `&nbsp&nbsp&nbsp&nbsp Latitude: ${`${hoverLatitude}`.slice(-12)}\u00B0` + `&nbsp&nbsp&nbsp&nbsp Elevation: ----`;
		
		// 本地查询
		context.searchPosition(hoverLongitude, hoverLatitude);
	  } else {
		hoverLongitude = 0.0;
		hoverLatitude = 0.0;
		geoinfo.innerHTML = `Longitude: ----` + `&nbsp&nbsp&nbsp&nbsp Latitude: ----` + `&nbsp&nbsp&nbsp&nbspElevation: ----`;
	  }
	}, Cesium.ScreenSpaceEventType.MOUSE_MOVE);
}

// 关闭鼠标浮动展示经纬度位置
function closeMousePicking() {
	let geoinfo = document.getElementById("geoinfo");
	geoinfo.style.visibility = "hidden";
	 
	if (undefined == glMouseOverHandler) return;
	
	// 关闭事件句柄
	glMouseOverHandler.destroy(); 
	glMouseOverHandler = undefined;
}