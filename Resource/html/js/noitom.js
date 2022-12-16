let context;
let cesiumViewer;
let measureStatus = 0;
let glMouseOverHandler = undefined;
let hoverLongitude = 0.0;
let hoverLatitude = 0.0;
let createMeasurePoint = false
let glSelectRemoteID = ""

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
				} else if (type === "remote point") {
					addRemotePointEntity(path)
				} else if (type === "remote tif") {
					addRemoteTiffEntity(path)
				}
			});
			context.sgl_change_entity_visible.connect(function(type, id, visible, parentId) {
				if (type === "kml" || type === "kmz") {
					if (parentId !== "") {
						changeDataSourceChildVisible(id, visible, parentId)
					} else {
						changeDataSourceVisible(id, visible, parentId)
					}
				} else if (type === "mla") {
					changeMeasureLineSourceVisible(id, visible)
				} else if (type === "mpa") {
					changeMeasurePolygnSourceVisible(id, visible)
				} else {
					// 隐藏、显示详细信息
					if (glSelectRemoteID === id) {
						let entityDescription = document.getElementById("entityDescription")
						entityDescription.style.visibility = visible ? "visible" : "hidden"
						
						let elementDivEmptyImage = document.getElementById("div-empty-image")
						elementDivEmptyImage.style.visibility = visible ? "visible" : "hidden"
					}
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
					// 隐藏详细信息
					if (glSelectRemoteID === id) {
						let entityDescription = document.getElementById("entityDescription")
						entityDescription.style.visibility = "hidden"
						
						let elementDivEmptyImage = document.getElementById("div-empty-image")
						elementDivEmptyImage.style.visibility = "hidden"
					}
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
	
	// 禁止右键菜单
	document.oncontextmenu = function() {
		event.returnValue = false
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
			glSelectRemoteID = ""
			if (undefined !== entityDescription) {
				entityDescription.style.visibility = "hidden";
				let elementDivEmptyImage = document.getElementById("div-empty-image");
				elementDivEmptyImage.style.visibility = "hidden" 
			}
		} else {
			if (undefined !== entityDescription) {
				let remoteObject = pick.id.remoteDescription;
				if (undefined === remoteObject) {
					glSelectRemoteID = ""
					entityDescription.style.visibility = "hidden";
				} else {
					// 记录选中 ID
					glSelectRemoteID = remoteObject.id
					
					let elementTextPriority = document.getElementById("text-priority");
					elementTextPriority.innerHTML = remoteObject.priority
					
					let elementTextId = document.getElementById("text-id");
					elementTextId.innerHTML = remoteObject.id
					elementTextId.parentNode.title = remoteObject.id
					
					let elementTextDtTime = document.getElementById("text-dt-time");
					let textDtTime = remoteObject.dt_time === "" ? "---" : remoteObject.dt_time
					elementTextDtTime.innerHTML = textDtTime
					elementTextDtTime.parentNode.title = textDtTime
					
					let elementTextLongitude = document.getElementById("text-longitude");
					let textLongitude = remoteObject.longitude === "" ? "---" : remoteObject.longitude
					elementTextLongitude.innerHTML = textLongitude
					elementTextLongitude.parentNode.title = textLongitude
					
					let elementTextLatitude = document.getElementById("text-latitude");
					let textLatitude = remoteObject.latitude === "" ? "---" : remoteObject.latitude
					elementTextLatitude.innerHTML = textLatitude
					elementTextLatitude.parentNode.title = textLatitude
					
					let elementTextDepth = document.getElementById("text-altitude");
					elementTextDepth.innerHTML = "---"
					elementTextDepth.parentNode.title = "---"
					
					let elementTextDtSpeed = document.getElementById("text-dt-speed");
					let textDtSpeed = remoteObject.dt_speed === "" ? "---" : remoteObject.dt_speed + " Knot"
					elementTextDtSpeed.innerHTML = textDtSpeed
					elementTextDtSpeed.parentNode.title = textDtSpeed
					
					let elementTextHorizontalRangeValue = document.getElementById("text-horizontal-range-value");
					let textHorizontalRangeValueA = remoteObject.horizontal_range_direction === "" ? "---" : remoteObject.horizontal_range_direction
					let textHorizontalRangeValueB = remoteObject.horizontal_range_value === "" ? "---" : remoteObject.horizontal_range_value + " 米"
					elementTextHorizontalRangeValue.innerHTML = textHorizontalRangeValueA + " / " + textHorizontalRangeValueB
					elementTextHorizontalRangeValue.parentNode.title = textHorizontalRangeValueA + " / " + textHorizontalRangeValueB
					
					let elementTextHeightFromBottom = document.getElementById("text-height-from-bottom");
					let textHeightFromBottom = remoteObject.height_from_bottom === "" ? "---" : remoteObject.height_from_bottom + " 米"
					elementTextHeightFromBottom.innerHTML = textHeightFromBottom
					elementTextHeightFromBottom.parentNode.title = textHeightFromBottom
					
					let elementTextRTheta = document.getElementById("text-r-theta");
					let textRTheta  = remoteObject.r_theta === "" ? "---" : remoteObject.r_theta
					elementTextRTheta.innerHTML = textRTheta
					elementTextRTheta.parentNode.title = textRTheta
					
					let elementTextAlongTrack = document.getElementById("text-along-track");
					let textAlongTrack  = remoteObject.along_track === "" ? "---" : remoteObject.along_track + " 米"
					elementTextAlongTrack.innerHTML = textAlongTrack
					elementTextAlongTrack.parentNode.title = textAlongTrack
					
					let elementTextAcrossTrack = document.getElementById("text-across-track");
					let textAcrossTrack  = remoteObject.across_track === "" ? "---" : remoteObject.across_track + " 米"
					elementTextAcrossTrack.innerHTML = textAcrossTrack
					elementTextAcrossTrack.parentNode.title = textAcrossTrack
					
					let elementTextRemarks = document.getElementById("text-remarks");
					let textRemarks  = remoteObject.remarks === "" ? "---" : remoteObject.remarks
					elementTextRemarks.innerHTML = textRemarks
					elementTextRemarks.parentNode.title = textRemarks
					
					let elementTextSupposeSize = document.getElementById("text-suppose-size");
					let textSupposeSize  = remoteObject.suppose_size === "" ? "---" : remoteObject.suppose_size + " 米"
					elementTextSupposeSize.innerHTML = textSupposeSize
					elementTextSupposeSize.parentNode.title = textSupposeSize
					
					// 查证信息
					let elementTextTargetLongitude = document.getElementById("text-target-longitude");
					let textTargetLongitude = remoteObject.target_longitude === "" ? "---" : remoteObject.target_longitude
					elementTextTargetLongitude.innerHTML = textTargetLongitude
					elementTextTargetLongitude.parentNode.title = textTargetLongitude
					
					let elementTextTargetLatitude = document.getElementById("text-target-latitude");
					let textTargetLatitude = remoteObject.target_latitude === "" ? "---" : remoteObject.target_latitude
					elementTextTargetLatitude.innerHTML = textTargetLatitude
					elementTextTargetLatitude.parentNode.title = textTargetLatitude
					
					let elementTextPositionError = document.getElementById("text-position-error");
					let textPositionError = remoteObject.position_error === "" ? "---" : remoteObject.position_error + " 米"
					elementTextPositionError.innerHTML = textPositionError
					elementTextPositionError.parentNode.title = textPositionError
					
					let elementTextCruiseNumber = document.getElementById("text-cruise-number");
					let textCruiseNumber = remoteObject.verify_cruise_number === "" ? "---" : remoteObject.verify_cruise_number
					elementTextCruiseNumber.innerHTML = textCruiseNumber
					elementTextCruiseNumber.parentNode.title = textCruiseNumber
					
					let elementTextDiveNumber = document.getElementById("text-dive-number");
					let textDiveNumber = remoteObject.verify_dive_number === "" ? "---" : remoteObject.verify_dive_number
					elementTextDiveNumber.innerHTML = textDiveNumber
					elementTextDiveNumber.parentNode.title = textDiveNumber
					
					let elementTextImageDescription = document.getElementById("text-image-description");
					let textImageDescription = remoteObject.image_description === "" ? "---" : remoteObject.image_description
					elementTextImageDescription.innerHTML = textImageDescription
					elementTextImageDescription.parentNode.title = textImageDescription
					
					let elementDivVerifyImage = document.getElementById("div-verify-image");
					
					let verifyImagePath = "";
					let verifyAuvPictureNumber = remoteObject.verify_auv_sss_image_paths.length
					for (let i = 0; i < verifyAuvPictureNumber; i++) {
						verifyImagePath += '<img src="' + remoteObject.verify_auv_sss_image_paths[i] + '" style="width: 100%; height: 14em; margin-bottom: 1em; border-radius: 6px; border: 2px solid #f0f0f0; object-fit: contain;">'
					}
					
					let verifyPictureNumber = remoteObject.verify_image_paths.length
					for (let i = 0; i < verifyPictureNumber;i++) {
						verifyImagePath += '<img src="' + remoteObject.verify_image_paths[i] + '" style="width: 100%; height: 14em; margin-bottom: 1em; border-radius: 6px; border: 2px solid #f0f0f0; object-fit: contain;">'
					}
					
					elementDivVerifyImage.innerHTML = verifyImagePath
					entityDescription.style.visibility = "visible";
					
					let elementDivEmptyImage = document.getElementById("div-empty-image");
					elementDivEmptyImage.style.visibility = verifyImagePath.length > 0 ? "hidden" : "visible";
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

// 添加 remote point 实体
function addRemotePointEntity(arg) {
	let remoteObject = eval("(" + arg + ")")
	if ((remoteObject.longitude == undefined) || (remoteObject.latitude === undefined)) return;
	cesiumViewer.entities.add({
		id: remoteObject.id,
		position: Cesium.Cartesian3.fromDegrees(remoteObject.longitude, remoteObject.latitude),
		point: {
		  color:  Cesium.Color.LIME ,
		  pixelSize: 8
		},
		remoteDescription: remoteObject
	})
	
	context.recvMsg("add", "remote point", true, remoteObject.id);
}

// 添加 remote tif 实体
function addRemoteTiffEntity(arg) {
	let remoteObject = eval("(" + arg + ")")
	if (undefined == remoteObject) {
		context.recvMsg("add", "remote tif", false, "元数据对象异常");
		return;
	}
	
	let xhr = new XMLHttpRequest();
	xhr.open('GET', remoteObject.side_scan_image_name);
	xhr.responseType = 'arraybuffer';
	xhr.onerror = function (error) {
		context.recvMsg("add", "remote tif", false, "图片加载失败");
	}
	xhr.onload = function (e) {
		if (e.total === 0) {
			// 表示加载失败，或文件不存在
			context.recvMsg("add", "remote tif", false, "目标图片不存在图片");
			return;
		}
		Tiff.initialize({TOTAL_MEMORY: parseInt(remoteObject.image_total_byte) * 1.1})
		let tiff = new Tiff({buffer: xhr.response});
		let canvas = tiff.toCanvas();
		let size = cesiumViewer.entities.values.length;
		cesiumViewer.entities.add({
		  id: remoteObject.id,
		  rectangle: {
			coordinates: Cesium.Rectangle.fromDegrees(parseFloat(remoteObject.image_top_left_longitude), parseFloat(remoteObject.image_top_left_latitude), parseFloat(remoteObject.image_bottom_right_longitude), parseFloat(remoteObject.image_bottom_right_latitude)),
			material: canvas,
			zIndex: size,
		  },
		  remoteDescription: remoteObject
		});
		context.recvMsg("add", "remote tif", true, remoteObject.id);
	};

	xhr.send();
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