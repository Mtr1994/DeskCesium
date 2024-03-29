let context;
let cesiumViewer;

// 初始化
function init() {
	if (typeof qt != 'undefined')
    {
        new QWebChannel(qt.webChannelTransport, function(channel) { 
			context = channel.objects.context; 
			context.sgl_add_remote_point_entitys.connect(function(obj) {
				addRemotePointEntitys(obj)
			});
			context.sgl_add_remote_trajectory_entitys.connect(function(obj) {
				addRemoteTrajectoryEntitys(obj)
			});
			
			context.recvWebMsg("init", true);
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
}

function handleDrop(event) {
	event.preventDefault();
}

function handleDragOver(event) {
	event.preventDefault();
}

// 添加 remote point 实体
function addRemotePointEntitys(obj) {
	if (obj.length === 0) return
	
	let remoteObject = eval("(" + obj + ")")
	if (remoteObject === undefined) return
	
	let errorPointArray = remoteObject.data
	let size = errorPointArray.length
	
	for (let i = 0; i < size; i++) {
		if (errorPointArray[i].priority === 0) continue;
		cesiumViewer.entities.add({
			position: Cesium.Cartesian3.fromDegrees(errorPointArray[i].longitude, errorPointArray[i].latitude),
			point: {
			  color: (errorPointArray[i].verify_flag == 0) ? Cesium.Color.CRIMSON : Cesium.Color.LIME,
			  pixelSize: 8 + errorPointArray[i].priority * 4
			},
			entityVerifyFlag: errorPointArray[i].verify_flag,
			entityPriority: errorPointArray[i].priority
		})
	}
	
	context.recvWebMsg("add", true);
}

// 添加 remote trajectory 实体
function addRemoteTrajectoryEntitys(obj) {
	if (obj.length === 0) return
	
	let remoteObject = eval("(" + obj + ")")

	if (undefined == remoteObject) {
		context.recvWebMsg("add", false)
		return
	}
	
	let lineColor = (remoteObject.type === "DT") ? Cesium.Color.CRIMSON : ((remoteObject.type === "AUV") ? Cesium.Color.DARKORANGE : ((remoteObject.type === "HOV") ? Cesium.Color.LIMEGREEN  : Cesium.Color.ORANGERED))
	let daraArray = remoteObject.trajectory_data
	let arraySize = daraArray.length
	for (let i = 0; i < arraySize; i++) {
		cesiumViewer.entities.add({
		  polyline: {
			positions: Cesium.Cartesian3.fromDegreesArrayHeights(daraArray[i]),
			width: 2,
			material: new Cesium.PolylineOutlineMaterialProperty({
			  color: lineColor,
			  outlineWidth: 1,
			  outlineColor: Cesium.Color.BLACK,
			})
		  },
		  entityType: remoteObject.type
		});
	}
}

function handleLegendClick (event) {
	let check = event.target.getAttribute("check")
	let checkColor = event.target.getAttribute("checkColor")
	let unCheckColor = "#cccccc"
	let legendType = event.target.getAttribute("legendType")
	let id = event.target.id
	
	if (check === "false") {
		event.target.setAttribute("check", "true")
		event.target.style.backgroundColor = checkColor
	} else {
		event.target.setAttribute("check", "false")
		event.target.style.backgroundColor = unCheckColor
	}
	
	let entitiyArray = cesiumViewer.entities.values
	let enditySize = entitiyArray.length
	for (let i = 0; i < enditySize; i++) {
		if (legendType === "priority") {
			let priority = (id === "p1") ? 1 : (id === "p2") ? 2 : (id === "p3") ? 3 : -1
			let entityPriority = entitiyArray[i].entityPriority
			if (undefined === entityPriority) continue
			if (priority !== entityPriority) continue
			
			// 判断是否查证
			let entityVerifyFlag = entitiyArray[i].entityVerifyFlag
			if(undefined === entityVerifyFlag) continue
			
			if (entityVerifyFlag === 0) {
				let elementUnVerifyFlag = document.getElementById("unverify")
				if (elementUnVerifyFlag.getAttribute("check") === "false") continue
			} else if (entityVerifyFlag === 1) {
				let elementUnVerifyFlag = document.getElementById("verify")
				if (elementUnVerifyFlag.getAttribute("check") === "false") continue
			}
			
			entitiyArray[i].show = (check === "false")
		} else if (legendType === "line") {
			let entityType = entitiyArray[i].entityType
			if (id !== entityType) continue;
			entitiyArray[i].show = (check === "false")
		} else if (legendType === "verifyFlag") {
			let verifyFlag = (id === "verify") ? 1 : (id === "unverify") ? 0 : -1
			let entityVerifyFlag = entitiyArray[i].entityVerifyFlag
			if (verifyFlag !== entityVerifyFlag) continue;
			
			// 判断优先级
			let entityPriority = entitiyArray[i].entityPriority
			if (entityPriority === 1) {
				let elementPriority1 = document.getElementById("p1")
				if (elementPriority1.getAttribute("check") === "false") continue
			} else if (entityPriority === 2) {
				let elementPriority2 = document.getElementById("p2")
				if (elementPriority2.getAttribute("check") === "false") continue
			} else if (entityPriority === 3) {
				let elementPriority3 = document.getElementById("p3")
				if (elementPriority3.getAttribute("check") === "false") continue
			}
			
			entitiyArray[i].show = (check === "false")
		}
	}
}

