// 初始化
function init() {
	if (typeof qt != 'undefined')
    {
        new QWebChannel(qt.webChannelTransport, function(channel) { 
			context = channel.objects.context; 
			context.sgl_change_preface_info.connect(function(obj) {
				changePrefaceInfo(obj)
			});
			
			context.recvWebMsg("init", true);
		});
    }
    else
    {
        alert("qt对象获取失败！");
    }
	
	// 禁止右键菜单
	document.oncontextmenu = function() {
		event.returnValue = false
	}
}

function handleDrop(event) {
	event.preventDefault();
}

function handleDragOver(event) {
	event.preventDefault();
}

// 修改 preface 信息
function changePrefaceInfo(obj) {
	if (obj.length === 0) return
	
	let remoteObject = eval("(" + obj + ")")
	if (remoteObject === undefined) return
	
	console.log("remoteObject", remoteObject)
	
	let elementTextTotalCruiseNumber = document.getElementById("text-total-cruise-number")
	elementTextTotalCruiseNumber.innerHTML = remoteObject.sidescan.total_cruise_number
	elementTextTotalCruiseNumber.title = remoteObject.sidescan.total_cruise_number
	
	let elementTextTotalLength = document.getElementById("text-total-length")
	elementTextTotalLength.innerHTML = parseFloat(remoteObject.cruiseroute.total_length).toFixed(2)
	elementTextTotalLength.title = parseFloat(remoteObject.cruiseroute.total_length).toFixed(2)
	
	let elementTextTotalArea = document.getElementById("text-total-area")
	elementTextTotalArea.innerHTML = parseFloat(remoteObject.cruiseroute.total_area).toFixed(2)
	elementTextTotalArea.title = parseFloat(remoteObject.cruiseroute.total_area).toFixed(2)
	
	let elementTextDtNumber = document.getElementById("text-total-dt-number")
	elementTextDtNumber.innerHTML = remoteObject.cruiseroute.total_dt_number
	elementTextDtNumber.title = remoteObject.cruiseroute.total_dt_number
	
	let elementTextDTLength = document.getElementById("text-dt-length")
	elementTextDTLength.innerHTML = parseFloat(remoteObject.cruiseroute.dt_total_length).toFixed(2)
	elementTextDTLength.title = parseFloat(remoteObject.cruiseroute.dt_total_length).toFixed(2)
	
	let elementTextDTArea = document.getElementById("text-dt-area")
	elementTextDTArea.innerHTML = parseFloat(remoteObject.cruiseroute.dt_total_area).toFixed(2)
	elementTextDTArea.title = parseFloat(remoteObject.cruiseroute.dt_total_area).toFixed(2)
	
	let elementTextAuvNumber = document.getElementById("text-total-auv-number")
	elementTextAuvNumber.innerHTML = remoteObject.cruiseroute.total_auv_number
	elementTextAuvNumber.title = remoteObject.cruiseroute.total_auv_number
	
	let elementTextAUVLength = document.getElementById("text-auv-length")
	elementTextAUVLength.innerHTML = parseFloat(remoteObject.cruiseroute.auv_total_length).toFixed(2)
	elementTextAUVLength.title = parseFloat(remoteObject.cruiseroute.auv_total_length).toFixed(2)
	
	let elementTextAUVVerifyNumber = document.getElementById("text-auv-verify-number")
	elementTextAUVVerifyNumber.innerHTML = remoteObject.sidescan.verify_auv
	elementTextAUVVerifyNumber.title = remoteObject.sidescan.verify_auv
		
	let elementTextHovNumber = document.getElementById("text-total-hov-number")
	elementTextHovNumber.innerHTML = remoteObject.cruiseroute.total_hov_number
	elementTextHovNumber.title = remoteObject.cruiseroute.total_hov_number
	
	let elementTextHovLength = document.getElementById("text-hov-length")
	elementTextHovLength.innerHTML = parseFloat(remoteObject.cruiseroute.hov_total_length).toFixed(2)
	elementTextHovLength.title = parseFloat(remoteObject.cruiseroute.hov_total_length).toFixed(2)
	
	let elementTextHOVVerifyNumber = document.getElementById("text-hov-verify-number")
	elementTextHOVVerifyNumber.innerHTML = remoteObject.sidescan.verify_hov
	elementTextHOVVerifyNumber.title = remoteObject.sidescan.verify_hov
	
	let elementTextTotalErrorPointNumber = document.getElementById("text-total-error-point-number")
	elementTextTotalErrorPointNumber.innerHTML = remoteObject.sidescan.total_error_number
	
	let elementTextTotalP1Number = document.getElementById("text-p1-number")
	elementTextTotalP1Number.innerHTML = remoteObject.sidescan.p1
	
	let elementTextTotalP2Number = document.getElementById("text-p2-number")
	elementTextTotalP2Number.innerHTML = remoteObject.sidescan.p2
	
	let elementTextTotalP3Number = document.getElementById("text-p3-number")
	elementTextTotalP3Number.innerHTML = remoteObject.sidescan.p3
	
	let elementTextTotalVerifyNumber = document.getElementById("text-total-verify-number")
	elementTextTotalVerifyNumber.innerHTML = remoteObject.sidescan.verify_number
	
	let elementTextTotalVerifyP1Number = document.getElementById("text-verify-p1-number")
	elementTextTotalVerifyP1Number.innerHTML = remoteObject.sidescan.verify_p1
	
	let elementTextTotalVerifyP2Number = document.getElementById("text-verify-p2-number")
	elementTextTotalVerifyP2Number.innerHTML = remoteObject.sidescan.verify_p2
	
	let elementTextTotalVerifyP3Number = document.getElementById("text-verify-p3-number")
	elementTextTotalVerifyP3Number.innerHTML = remoteObject.sidescan.verify_p3
	
	
	
	
}

