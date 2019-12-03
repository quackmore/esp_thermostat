import * as esp from "./esp_queries.js";

//  CURRENT VALUES

setInterval(update_collapseCurrent, 15000);

$(document).ready(function () {
  update_collapseCurrent();
});

$('#collapseCurrent').on('show.bs.collapse', function () {
  update_collapseCurrent();
});

function update_collapseCurrent() {
  esp.get_current_vars(update_current_vars, function (xhr) {
    alert("" + xhr.responseText);
  });
}

function update_current_vars(data) {
  let week_day = new Array("Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun");
  $("#thermo_date").text(function () {
    let date = new Date(data.ctrl_date * 1000);
    return week_day[date.getDay()] +
      " " +
      (date.getHours() > 9 ? date.getHours() : "0" + date.getHours()) +
      ":" +
      (date.getMinutes() > 9 ? date.getMinutes() : "0" + date.getMinutes());
  });
  if (data.current_temp == -500)
    $("#thermo_temp").text("--.-");
  else
    $("#thermo_temp").text((data.current_temp / 10).toFixed(1));
  $("#thermo_heater").text(function () {
    if (data.heater_status == 0)
      return "heater OFF";
    else
      return "heater ON";
  });
  $("#thermo_temp_target").text(function () {
    if (data.ctrl_mode == 2)
      return "setpoint " + (data.auto_setpoint / 10).toFixed(1);
    else
      return "no setpoint";
  });
  let mode = new Array("OFF", "MANUAL", "AUTO");
  $("#thermo_mode").text(function () {
    return "mode " + mode[data.ctrl_mode];
  });
  if (data.pwr_off_timer == 0) {
    $("#thermo_pwr_off").addClass('d-none');
  } else {
    $("#thermo_pwr_off").removeClass('d-none');
    let now = new Date();
    let timer_started = new Date(data.pwr_off_timer_started_on * 1000);
    let pwr_off_minutes = Math.floor((now - timer_started) / 60000);
    if (((now - timer_started) % 60000) > 1000)
      pwr_off_minutes += 1;
    pwr_off_minutes = data.pwr_off_timer - pwr_off_minutes + 1;
    if (pwr_off_minutes < 0)
      $("#thermo_pwr_off").addClass('d-none');
    else
      $("#thermo_pwr_off").text(function () {
        return "power off in " + pwr_off_minutes + " minutes";
      });
  }
}

// CONTROL SETTINGS

$('#collapseSettings').on('show.bs.collapse', function () {
  update_collapseSettings();
});

$('#off_timer').change(function () {
  $('#off_timer').val(Math.floor($('#off_timer').val()))
});

$('#cycle_on').change(function () {
  $('#cycle_on').val(Math.floor($('#cycle_on').val()))
});

$('#cycle_off').change(function () {
  $('#cycle_off').val(Math.floor($('#cycle_off').val()))
});

$('#ctrl_mode').change(function () {
  refresh_settings_view();
});

$('#cont_cycle').change(function () {
  refresh_settings_view();
});

$('#off_timer_en').change(function () {
  refresh_settings_view();
});

$('#settings_reset').click(function () {
  update_collapseSettings();
});

$('#settings_save').click(function () {
  esp.save_settings(jsonify_settings,
    function (xhr) {
      alert("" + xhr.responseText);
    }, function (xhr) {
      alert("" + xhr.responseText);
    });
});

function update_collapseSettings() {
  esp.get_settings(update_settings, function (xhr) {
    alert("" + xhr.responseText);
  });
}

function update_settings(data) {
  // mode: 2,
  // manual_pulse_on: 0,
  // manual_pulse_off: 0,
  // auto_setpoint: 200,
  // pwr_off: 30
  $('#ctrl_mode').val(data.ctrl_mode);
  if (data.manual_pulse_on == 0)
    $('#cont_cycle').val(0);
  else
    $('#cont_cycle').val(1);
  $('#cycle_on').val(data.manual_pulse_on);
  $('#cycle_off').val(data.manual_pulse_off);
  $('#setpoint').val((data.auto_setpoint / 10).toFixed(1));
  if (data.pwr_off_timer == 0)
    $('#off_timer_en').val(0);
  else
    $('#off_timer_en').val(1);
  $('#off_timer').val(data.pwr_off_timer);
  refresh_settings_view();
}

function refresh_settings_view() {
  if ($('#ctrl_mode').val() == 0) {
    $("#manual_settings").addClass('d-none');
    $("#manual_pulse_on").addClass('d-none');
    $("#manual_pulse_off").addClass('d-none');
    $("#auto_setpoint").addClass('d-none');
    $("#power_off_timer").addClass('d-none');
  }
  if ($('#ctrl_mode').val() == 1) {
    $("#manual_settings").removeClass('d-none');
    if ($('#cont_cycle').val() == 0) {
      $("#manual_pulse_on").addClass('d-none');
      $("#manual_pulse_off").addClass('d-none');
    }
    else {
      $("#manual_pulse_on").removeClass('d-none');
      $("#manual_pulse_off").removeClass('d-none');
    }
    $("#auto_setpoint").addClass('d-none');
    $("#power_off_timer").removeClass('d-none');
    if ($("#off_timer_en").val() == 0)
      $("#off_timer").attr('readonly', true);
    else
      $("#off_timer").attr('readonly', false);
  }
  if ($('#ctrl_mode').val() == 2) {
    $("#manual_settings").addClass('d-none');
    $("#manual_pulse_on").addClass('d-none');
    $("#manual_pulse_off").addClass('d-none');
    $("#auto_setpoint").removeClass('d-none');
    $("#power_off_timer").removeClass('d-none');
    if ($("#off_timer_en").val() == 0)
      $("#off_timer").attr('readonly', true);
    else
      $("#off_timer").attr('readonly', false);
  }
}

function jsonify_settings() {
  var to_be_saved_settings = new Object;
  to_be_saved_settings.ctrl_mode = parseInt($('#ctrl_mode').val());
  if ($('#cont_cycle').val() == 0)
    to_be_saved_settings.manual_pulse_on = 0;
  else
    to_be_saved_settings.manual_pulse_on = parseInt($('#cycle_on').val());
  to_be_saved_settings.manual_pulse_off = parseInt($('#cycle_off').val());
  to_be_saved_settings.auto_setpoint = parseFloat($('#setpoint').val()) * 10;
  if ($('#off_timer_en').val() == 0)
    to_be_saved_settings.power_off_timer = 0;
  else
    to_be_saved_settings.power_off_timer = parseInt($('#off_timer').val());
  return to_be_saved_settings;
}

// REMOTE LOG SETTINGS

$('#collapseRemoteLogSettings').on('show.bs.collapse', function () {
  esp.get_rl_settings(update_remoteLogSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#rl_reset').click(function () {
  esp.get_rl_settings(update_remoteLogSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#rl_save').click(function () {
  esp.save_rl_settings(jsonify_rl_settings,
    function (xhr) {
      alert("" + xhr.responseText);
    }, function (xhr) {
      alert("" + xhr.responseText);
    });
});

function update_remoteLogSettings(data) {
  if (data.enabled == 0)
    $('#rl_enabled').val(0);
  else
    $('#rl_enabled').val(1);
  $('#rl_host').val(data.host);
  $('#rl_port').val(data.port);
  $('#rl_path').val(data.path);
}

function jsonify_rl_settings() {
  var to_be_saved_rl_settings = new Object;
  to_be_saved_rl_settings.enabled = parseInt($('#rl_enabled').val());
  to_be_saved_rl_settings.host = $('#rl_host').val();
  to_be_saved_rl_settings.port = parseInt($('#rl_port').val());
  to_be_saved_rl_settings.path = $('#rl_path').val();
  return to_be_saved_rl_settings;
}