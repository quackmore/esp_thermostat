// command.js

// spinner while awaiting for page load
$(document).ready(function () {
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_page();
});

function update_page() {
  esp_get_history(formatData);
  setTimeout(function () {
    createChart();
  }, 500);
}

$('#history_refresh').on('click', function () {
  $('#awaiting').modal('show');
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_page();
});

function sort_data(array) {
  return Promise.resolve(array.sort(function (a, b) { return (a.ts - b.ts); }));
}

function esp_get_history() {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/ctrl/log',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      sort_data(data.ctrl_events)
        .then(formatData(data.ctrl_events));
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

// 
// charts
// 

var temperatureData = [];

var humidityData = [];
var min_humy = 0;
var max_humy = 100;
var min_temp = -20;
var max_temp = 100;


var spData = [];

var heaterData = [];


function formatData(data) {
  temperatureData = [];

  var startDate = ((new Date()).getTime() / 1000) - (30 * 24 * 60 * 60);
  // eliminate too old data
  var idx = 0;
  while (data[idx].ts < startDate) {
    data.shift();
  }
  startDate = data[0].ts;
  endDate = data[data.length - 1].ts;
  // split data
  temperatureData = [];
  data.forEach(function (element) {
    if (element.tp == 1)
      temperatureData.push({ x: new Date(element.ts * 1000), y: element.vl / 10 });
  });
  humidityData = [];
  data.forEach(function (element) {
    if (element.tp == 5)
      humidityData.push({ x: new Date(element.ts * 1000), y: element.vl / 10 });
  });
  spData = [];
  data.forEach(function (element) {
    if (element.tp == 4)
      spData.push({ x: new Date(element.ts * 1000), y: element.vl / 10 });
  });
  heaterData = [];
  data.forEach(function (element) {
    if (element.tp == 2)
      heaterData.push({ x: new Date(element.ts * 1000), y: element.vl });
  });
  // scale min/max
  if (temperatureData.length > 0) {
    if (temperatureData[0].x > (new Date(startDate * 1000))) {
      temperatureData.unshift({ x: new Date(startDate * 1000), y: temperatureData[0].y });
    }
    if (temperatureData[temperatureData.length - 1].x < (new Date(endDate * 1000))) {
      temperatureData.push({ x: new Date(endDate * 1000), y: temperatureData[temperatureData.length - 1].y });
    }
    min_temp = min_array_val(temperatureData);
    max_temp = max_array_val(temperatureData);
    min_sp = min_array_val(spData);
    max_sp = max_array_val(spData);
    if (min_temp > min_sp)
      min_temp = min_sp;
    if (max_temp < max_sp)
      max_temp = max_sp;
    min_temp = min_temp - (max_temp - min_temp) / 5;
    max_temp = max_temp + (max_temp - min_temp) / 2;
  }
  if (humidityData.length > 0) {
    if (humidityData[0].x > (new Date(startDate * 1000))) {
      humidityData.unshift({ x: new Date(startDate * 1000), y: humidityData[0].y });
    }
    if (humidityData[humidityData.length - 1].x < (new Date(endDate * 1000))) {
      humidityData.push({ x: new Date(endDate * 1000), y: humidityData[humidityData.length - 1].y });
    }
    min_humy = min_array_val(humidityData);
    if (min_humy < 0)
      min_humy = 0;
    else
      min_humy = min_humy / 4;
    max_humy = max_array_val(humidityData);
    if (max_humy > 99)
      max_humy = 110;
    else
      max_humy = 100 - ((100 - max_humy) / 1.5);
  }
  if (spData.length > 0) {
    if (spData[0].x > (new Date(startDate * 1000))) {
      spData.unshift({ x: new Date(startDate * 1000), y: min_temp });
    }
    if (spData[spData.length - 1].x < (new Date(endDate * 1000))) {
      spData.push({ x: new Date(endDate * 1000), y: spData[spData.length - 1].y });
    }
  }

  if (heaterData.length > 0) {
    if (heaterData[0].x > (new Date(startDate * 1000))) {
      heaterData.unshift({ x: new Date(startDate * 1000), y: opposite_heater_status(heaterData[0].y) });
    }
    if (heaterData[heaterData.length - 1].x < (new Date(endDate * 1000))) {
      heaterData.push({ x: new Date(endDate * 1000), y: heaterData[heaterData.length - 1].y });
    }
  }
}

function opposite_heater_status(status) {
  if (status == 0)
    return 1;
  else
    return 0;
}

var ctx = $('#historyChart');
var myChart;

function createChart() {
  myChart = new Chart(ctx, {
    type: 'scatter',
    data: {
      datasets: [{
        label: "Temperature",
        data: temperatureData,
        pointRadius: 0,
        showLine: true,
        fill: false,
        borderColor: "green",
        borderWidth: 1,
        yAxisID: 'temp-axis'
      }, {
        label: "Humidity",
        data: humidityData,
        pointRadius: 0,
        showLine: true,
        fill: false,
        borderColor: "blue",
        borderWidth: 1,
        yAxisID: 'humy-axis'
      }, {
        label: "Setpoint",
        data: spData,
        pointRadius: 0,
        showLine: true,
        steppedLine: true,
        fill: false,
        borderColor: "red",
        borderWidth: 1,
        yAxisID: 'temp-axis'
      }, {
        label: "Heater",
        data: heaterData,
        pointRadius: 0,
        showLine: true,
        steppedLine: true,
        fill: false,
        borderColor: "black",
        borderWidth: 1,
        yAxisID: 'heater-axis'
      }],
    },
    options: {
      legend: {
        display: true,
        position: 'bottom'
      },
      scales: {
        xAxes: [{
          type: 'time',
          time: {
            displayFormats: {
              minute: 'HH:mm'
            }
          }
        }],
        yAxes: [{
          id: 'temp-axis',
          type: 'linear',
          position: 'left',
          ticks: {
            min: min_temp,
            max: max_temp
          }
        }, {
          id: 'humy-axis',
          type: 'linear',
          position: 'right',
          ticks: {
            min: min_humy,
            max: max_humy
          }
        }, {
          id: 'heater-axis',
          type: 'linear',
          position: 'right',
          ticks: {
            min: 0,
            max: 3,
            display: false
          },
          gridLines: {
            display: false,
            drawBorder: false
          }
        }]
      },
      responsive: true
    }
  });
}

function max_array_val(array) {
  var max_val = -500;
  array.forEach(function (element) {
    if (element.y > max_val)
      max_val = element.y;
  });
  return max_val;
}

function min_array_val(array) {
  var min_val = 500;
  array.forEach(function (element) {
    if (element.y < min_val)
      min_val = element.y;
  });
  return min_val;
}