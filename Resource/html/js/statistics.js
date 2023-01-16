let myChart
let chartOption;

// 初始化
function init() {
	if (typeof qt != 'undefined')
    {
        new QWebChannel(qt.webChannelTransport, function(channel) { 
			context = channel.objects.context; 
			context.sgl_load_year_curve_chart.connect(function(obj) {
				addYearCurveChart(obj)
			});
			context.sgl_load_priority_pie_chart.connect(function(obj) {
				addPriorityPieChart(obj);
			});
			context.sgl_load_check_pie_chart.connect(function(obj) {
				addCheckedPieChart(obj);
			});
			
			context.recvWebMsg("init", "success");
		});
    }
    else
    {
        alert("can not find qt web channel");
    }
	
	var chartDom = document.getElementById('main');
	myChart = echarts.init(chartDom);
	
	window.onresize = myChart.resize;
}

function handleDrop(event) {
	event.preventDefault();
}

function handleDragOver(event) {
	event.preventDefault();
}

// 添加 年份 折线图
function addYearCurveChart(obj) {
	if (obj.length === 0) return
	
	let jsObject = eval("(" + obj + ")")
	if (jsObject == undefined)  return;
	
	let cruiseYearData = jsObject.curise_year_data
	let size = cruiseYearData.length
	let xAxisArray = []
	let seriesDataArray = []
	for (let i = 0; i < size; i++) {
		xAxisArray.push(cruiseYearData[i].year)
		seriesDataArray.push(cruiseYearData[i].number)
	}
	
	chartOption = {
		title: {
			text: '年份统计',
			subtext: '*',
			left: 'center',
			bottom: 'bottom'
		},
		tooltip: {
			trigger: 'item'
		},
		xAxis: {
			type: 'category',
			data: xAxisArray
		},
		yAxis: {
			type: 'value'
		},
		series: [
		{
			data: seriesDataArray,
			type: 'line',
			smooth: true
		}
		]
	};
	
	chartOption && myChart.setOption(chartOption);
}

// 添加 优先级饼图
function addPriorityPieChart(obj) {
	if (obj.length === 0) return
	
	let jsObject = eval("(" + obj + ")")
	if (jsObject == undefined) return;
	
	let priorityData = jsObject.priority_data
	
	chartOption = {
		title: {
			text: '优先级比例',
			left: 'center',
			bottom: 'bottom'
		},
		tooltip: {
			trigger: 'item'
		},
		legend: {
			orient: 'vertical',
			right: 'right'
		},
		series: [
		{
			name: '-',
			type: 'pie',
			radius: '50%',
			label: {
				show: false,
			},
			data: priorityData
		}
		]
	};
	
	chartOption && myChart.setOption(chartOption);
}

// 添加 查证饼图
function addCheckedPieChart(obj) {
	if (obj.length === 0) return
	
	let jsObject = eval("(" + obj + ")")
	if (jsObject == undefined) return;
	
	let verifyData = jsObject.verify_data
	
	chartOption = {
		title: {
			text: '查证比例',
			left: 'center',
			bottom: 'bottom'
		  },
		  tooltip: {
			trigger: 'item'
		  },
		  legend: {
			orient: 'vertical',
			right: 'right'
		  },
		  series: [
			{
			  name: '-',
			  type: 'pie',
			  radius: '50%',
			  label: {
				show: false,
			  },
			  data: verifyData,
			  emphasis: {
				itemStyle: {
				  shadowBlur: 10,
				  shadowOffsetX: 0,
				  shadowColor: 'rgba(0, 0, 0, 0.5)'
				}
			  }
			}
		  ]
	};
	
	chartOption && myChart.setOption(chartOption);
}