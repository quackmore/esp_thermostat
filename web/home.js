// home.js

$(document).ready(function () {
  esp_get_info()
    .then(function (data) {
      $('#content-title').text(data.device_name);
      update_page();
    });
});

function esp_get_info() {
  return esp_query({
    type: 'GET',
    url: '/api/info',
    dataType: 'json',
    success: null,
    error: query_err
  });
}

function update_page() {
  // this runs periodically 
  // so check that the page was not unloaded
  // before updating
  if ($('#sm_time').length == 0)
    return;
  esp_get_ctrl_vars()
    .then(function () {
      hide_spinner(500);
    });
  setTimeout(function () {
    update_page();
  }, 15000);
}

// Summary

function esp_get_ctrl_vars() {
  return esp_query({
    type: 'GET',
    url: '/api/ctrl/vars',
    dataType: 'json',
    success: update_summary,
    error: query_err
  });
}

var heater_ctrl_paused = 0;

function update_summary(data) {
  // day, time, temperature and relative humidity
  var week_day = new Array("Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun");
  $("#sm_time").text(function () {
    var date = new Date(data.timestamp * 1000);
    date = new Date(date.getTime() + date.getTimezoneOffset() * 60000 + data.timezone * 60 * 60000);

    return week_day[date.getDay()] +
      " " +
      (date.getHours() > 9 ? date.getHours() : "0" + date.getHours()) +
      ":" +
      (date.getMinutes() > 9 ? date.getMinutes() : "0" + date.getMinutes());
  });
  if (data.current_temp == -500)
    $("#sm_temp").text("--.-");
  else
    $("#sm_temp").html((data.current_temp / 10).toFixed(1) + "°<small> C</small>");
  if (data.current_humi == -500)
    $("#sm_humi").text("--.-");
  else
    $("#sm_humi").html((data.current_humi / 10).toFixed(1) + "<small> %</small>");
  // thermostat mode a current status
  $("#sm_heater_status").text(function () {
    if (data.heater_status == 0)
      return "heater OFF";
    else
      return "heater ON";
  });
  var mode = new Array("OFF", "MANUAL", "AUTO", "PROGRAM");
  $("#sm_ctrl_mode").text(function () {
    return "mode " + mode[data.ctrl_mode];
  });
  if ((data.ctrl_mode == 2) || (data.ctrl_mode == 3)) {
    $('#sm_ctrl_sp').html("<small>set-point</small> " + (data.auto_setpoint / 10).toFixed(1) + "°<small> C</small>");
    $('#sm_ctrl_sp').removeClass('d-none');

  } else {
    $('#sm_ctrl_sp').addClass('d-none');
  }
  if (data.ctrl_mode == 3) {
    $('#sm_ctrl_program').text(data.program_name);
    $('#sm_ctrl_program').removeClass('d-none');

  } else {
    $('#sm_ctrl_program').addClass('d-none');
  }
  // power off status  
  if (data.pwr_off_timer == 0) {
    $("#sm_pwr_off").addClass('d-none');
  } else {
    var now = new Date();
    var timer_started = new Date(data.pwr_off_timer_started_on * 1000);
    var pwr_off_minutes = Math.floor((now - timer_started) / 60000);
    if (((now - timer_started) % 60000) > 1000)
      pwr_off_minutes += 1;
    pwr_off_minutes = data.pwr_off_timer - pwr_off_minutes + 1;
    if (pwr_off_minutes <= 0)
      $("#sm_pwr_off").addClass('d-none');
    else {
      $("#sm_pwr_off").removeClass('d-none');
      $("#sm_ctrl_pwr_off").text(pwr_off_minutes);
    }
  }
  // heater control pause 
  if (data.ctrl_mode == 0) {
    // mode OFF
    $("#heater_ctrl").addClass('d-none');
  } else {
    $("#heater_ctrl").removeClass('d-none');
    // check if control is suspended
    if (data.ctrl_paused == 0) {
      heater_ctrl_paused = 0;
      $("#heater_ctrl_txt").text("Heater Control Active");
      $("#heater_ctrl_btn").html('<i class="fa fa-pause fa-2x" aria-hidden="true"></i>');
    } else {
      heater_ctrl_paused = 1;
      $("#heater_ctrl_txt").text("Heater Control Suspended");
      $("#heater_ctrl_btn").html('<i class="fa fa-play fa-2x" aria-hidden="true"></i>');
    }
  }
}

function ctrl_pause(status) {
  return esp_query({
    type: 'POST',
    url: '/api/ctrl/pause',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ ctrl_paused: status }),
    success: null,
    error: query_err
  });
}

$('#heater_ctrl_btn').on('click', function () {
  show_spinner().then(function () {
    var new_status;
    if (heater_ctrl_paused == 0)
      new_status = 1;
    else
      new_status = 0;
    ctrl_pause(new_status).then(function () {
      esp_get_ctrl_vars().then(function () {
        hide_spinner(500)
      })
    })
  })
});