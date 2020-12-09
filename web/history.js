// command.js

// spinner while awaiting for page load
$(document).ready(function () {
  esp_get_history()
    .then(function () {
      hide_spinner(800)
    });
});

$('#history_refresh').on('click', function () {
  show_spinner()
    .then(function () {
      esp_get_history()
        .then(function () {
          hide_spinner(800)
        });
    });
});

function esp_get_history() {
  return esp_query({
    type: 'GET',
    url: '/api/ctrl/log',
    dataType: 'json',
    success: function (data) {
      Promise.resolve()
        .then(function () {
          return Promise.resolve(data.ctrl_events.sort(function (a, b) { return (a.ts - b.ts); }));
        })
        .then(function () {
          return Promise.resolve(formatData(data.ctrl_events));
        })
        .then(function () {
          createChart();
        })
    },
    error: query_err
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
  if (data.length == 0)
    return;
  //  x values: state the time range
  var startDate = data[0].ts;
  var newStartDate = startDate;
  var endDate = data[data.length - 1].ts;
  // check if lags exist between data timestamp
  // pick data with the latest timestamp range 
  const oneYear = 60 * 60 * 24 * 365;
  var tsLagsExists = 0;
  var idx = 0;
  while (idx < data.length) {
    if ((data[idx].ts - startDate) > oneYear) {
      // update new start date
      newStartDate = data[idx].ts;
      tsLagsExists = 1;
      break;
    }
    idx++;
  }
  if (tsLagsExists) {
    // uddate start date cause old data will be discarded
    startDate = newStartDate;
  }
  // if latest data is less than one day old use current time as end date
  if ((((new Date()).getTime() / 1000) - endDate) < (60 * 60 * 24))
    endDate = (new Date()).getTime() / 1000;
  // if all timestamps are the same... add 1 second to end date 
  if (startDate == endDate)
    endDate = startDate + 1;
  // y values
  // split data for different lines
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
  // temperature data
  if (temperatureData.length > 0) {
    // scale min/max
    min_temp = min_array_val(temperatureData);
    max_temp = max_array_val(temperatureData);
    // check for invalid data
    if (min_temp == -500)
      min_temp = 0;
    if (max_temp == -500)
      max_temp = 100;
    // checkout for temperature setpoint range too
    if (spData.length > 0) {
      min_sp = min_array_val(spData);
      max_sp = max_array_val(spData);
    } else {
      min_sp = min_temp;
      max_sp = max_temp;
    }
    if (min_temp > min_sp)
      min_temp = min_sp;
    if (max_temp < max_sp)
      max_temp = max_sp;
    // state range and limits  
    delta_temp = max_temp - min_temp;
    if (delta_temp == 0)
      delta_temp = 4;
    min_temp = min_temp - delta_temp / 10;
    max_temp = max_temp + delta_temp / 4;
    // discard values out of the selected time range
    var startingTemp = temperatureData[0].y;
    if (tsLagsExists) {
      while (temperatureData.length && temperatureData[0].x < (new Date(startDate * 1000))) {
        startingTemp = temperatureData[0].y;
        temperatureData.shift();
      }
    }
    // extend the line over the whole time range
    if (temperatureData.length > 0) {
      // insert starting point (when needed)
      if (temperatureData[0].x > (new Date(startDate * 1000))) {
        temperatureData.unshift({ x: new Date(startDate * 1000), y: startingTemp });
      }
      // insert ending point (when needed)
      if (temperatureData[temperatureData.length - 1].x < (new Date(endDate * 1000))) {
        temperatureData.push({ x: new Date(endDate * 1000), y: temperatureData[temperatureData.length - 1].y });
      }
    }
    else {
      // when only one point exists...
      temperatureData.unshift({ x: new Date(startDate * 1000), y: startingTemp });
      temperatureData.push({ x: new Date(endDate * 1000), y: temperatureData[temperatureData.length - 1].y });
    }
  }
  // humidity
  if (humidityData.length > 0) {
    // scale min/max
    min_humy = min_array_val(humidityData);
    max_humy = max_array_val(humidityData);
    // check fo invalid data
    if (min_humy == -500)
      min_humy = 0;
    if (max_humy == -500)
      max_humy = 100;
    // state range and limits  
    delta_humy = max_humy - min_humy;
    if (delta_humy == 0)
      delta_humy = 5;
    min_humy = min_humy - delta_humy / 4;
    max_humy = max_humy + delta_humy / 10;
    // discard values out of the selected time range
    var startingHumy = humidityData[0].y;
    if (tsLagsExists) {
      while (humidityData.length && humidityData[0].x < (new Date(startDate * 1000))) {
        startingHumy = humidityData[0].y;
        humidityData.shift();
      }
    }
    // extend the line over the whole time range
    if (humidityData.length > 0) {
      // insert starting point (when needed) 
      if (humidityData[0].x > (new Date(startDate * 1000))) {
        humidityData.unshift({ x: new Date(startDate * 1000), y: startingHumy });
      }
      // insert ending point (when needed)
      if (humidityData[humidityData.length - 1].x < (new Date(endDate * 1000))) {
        humidityData.push({ x: new Date(endDate * 1000), y: humidityData[humidityData.length - 1].y });
      }
    }
    else {
      // when only one point exists...
      humidityData.unshift({ x: new Date(startDate * 1000), y: startingHumy });
      humidityData.push({ x: new Date(endDate * 1000), y: humidityData[humidityData.length - 1].y });
    }
  }
  // temperature setpoint
  if (spData.length > 0) {
    // discard values out of the selected time range
    var startingSp = spData[0].y;
    if (tsLagsExists) {
      while (spData.length && spData[0].x < (new Date(startDate * 1000))) {
        startingSp = spData[0].y;
        spData.shift();
      }
    }
    // extend the line till the end of the time range
    // don't extend to the start of the time range, it could be misleading
    if (spData.length > 0) {
      // insert ending point (when needed)
      if (spData[spData.length - 1].x < (new Date(endDate * 1000))) {
        spData.push({ x: new Date(endDate * 1000), y: spData[spData.length - 1].y });
      }
    }
    else {
      // when only one point exists...
      spData.unshift({ x: new Date(startDate * 1000), y: startingSp });
      spData.push({ x: new Date(endDate * 1000), y: spData[spData.length - 1].y });
    }
  }
  // heater status
  if (heaterData.length > 0) {
    // discard values out of the selected time range
    var startingHeater = heaterData[0].y;
    if (tsLagsExists) {
      while (heaterData.length && heaterData[0].x < (new Date(startDate * 1000))) {
        startingHeater = heaterData[0].y;
        heaterData.shift();
      }
    }
    // extend the line over the whole time range
    if (heaterData.length > 0) {
      // insert starting point (when needed)
      if (heaterData[0].x > (new Date(startDate * 1000))) {
        heaterData.unshift({ x: new Date(startDate * 1000), y: opposite_heater_status(heaterData[0].y) });
      }
      // insert ending point (when needed)
      if (heaterData[heaterData.length - 1].x < (new Date(endDate * 1000))) {
        heaterData.push({ x: new Date(endDate * 1000), y: heaterData[heaterData.length - 1].y });
      }
    }
    else {
      // when only one point exists...
      heaterData.unshift({ x: new Date(startDate * 1000), y: startingHeater });
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
        steppedLine: true,
        fill: false,
        borderColor: "green",
        borderWidth: 1,
        yAxisID: 'temp-axis'
      }, {
        label: "Humidity",
        data: humidityData,
        pointRadius: 0,
        showLine: true,
        steppedLine: true,
        //        lineTension: 0,
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
            max: 5,
            display: false
          },
          gridLines: {
            display: false,
            drawBorder: false
          }
        }]
      },
      plugins: {
        zoom: {
          pan: {
            enabled: true,
            mode: 'x'
          },
          zoom: {
            enabled: true,
            mode: 'x'
          }
        }
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