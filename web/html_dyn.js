const esp8266 = {
  "url": "http://192.168.1.105",
  // "url": "http://192.168.1.185",
  "cors": true
  // "url": "",
  // "cors": false
};

function get_current_vars(success_cb, error_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/temp_ctrl_vars',
    dataType: 'json',
    crossDomain: esp8266.cors,
    //success: success_cb(data),
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
  // let data = {
  //   ctrl_date: 1573157694,
  //   current_temp: 200,
  //   heater_status: 0,
  //   auto_setpoint: 200,
  //   ctrl_mode: 2,
  //   pwr_off_timer_started_on: 1573206000,
  //   pwr_off_timer: 90
  // };
  // success_cb(data);
}

function get_settings(success_cb, error_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/temp_ctrl_settings',
    dataType: 'json',
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
  // let data = {
  //   ctrl_mode: 1,
  //   manual_pulse_on: 0,
  //   manual_pulse_off: 0,
  //   auto_setpoint: 200,
  //   pwr_off_timer: 30
  // };
  // success_cb(data);
}

function save_settings(get_data, success_cb, error_cb) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/temp_ctrl_settings',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify(get_data()),
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  })
}

function get_rl_settings(success_cb, error_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/remote_log_settings',
    dataType: 'json',
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
}

function save_rl_settings(get_data, success_cb, error_cb) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/remote_log_settings',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify(get_data()),
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  })
}

function get_adv_ctrl_settings(success_cb, error_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/temp_ctrl_adv_settings',
    dataType: 'json',
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
}

function save_adv_ctrl_settings(get_data, success_cb, error_cb) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/temp_ctrl_adv_settings',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify(get_data()),
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  })
}

//  CURRENT VALUES

setInterval(update_collapseCurrent, 15000);

$(document).ready(function () {
  update_collapseCurrent();
});

$('#collapseCurrent').on('show.bs.collapse', function () {
  update_collapseCurrent();
});

function update_collapseCurrent() {
  get_current_vars(update_current_vars, function (xhr) {
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
  save_settings(jsonify_settings,
    function (xhr) {
      alert("" + xhr.responseText);
    }, function (xhr) {
      alert("" + xhr.responseText);
    });
});

function update_collapseSettings() {
  get_settings(update_settings, function (xhr) {
    alert("" + xhr.responseText);
  });
}

function update_settings(data) {
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

// ADVANCED CONTROL SETTINGS

$('#collapseAdvSettings').on('show.bs.collapse', function () {
  get_adv_ctrl_settings(update_advCtrlSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#adv_settings_reset').click(function () {
  get_adv_ctrl_settings(update_advCtrlSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#adv_settings_save').click(function () {
  save_adv_ctrl_settings(jsonify_advCtrlSettings,
    function (xhr) {
      alert("" + xhr.responseText);
    }, function (xhr) {
      alert("" + xhr.responseText);
    });
});

function update_advCtrlSettings(data) {
  $('#adv_sett_kp').val(data.kp);
  $('#adv_sett_kd').val(data.kd);
  $('#adv_sett_ki').val(data.ki);
  $('#adv_sett_u_max').val(data.u_max);
  $('#adv_sett_heater_on_min').val(data.heater_on_min);
  $('#adv_sett_heater_on_max').val(data.heater_on_max);
  $('#adv_sett_heater_on_off').val(data.heater_on_off);
  $('#adv_sett_heater_cold').val(data.heater_cold);
  $('#adv_sett_warm_up_period').val(data.warm_up_period);
  $('#adv_sett_wup_heater_on').val(data.wup_heater_on);
  $('#adv_sett_wup_heater_off').val(data.wup_heater_off);
}

function jsonify_advCtrlSettings() {
  var to_be_saved_adv_settings = new Object;
  to_be_saved_adv_settings.kp = parseInt($('#adv_sett_kp').val());
  to_be_saved_adv_settings.kd = parseInt($('#adv_sett_kd').val());
  to_be_saved_adv_settings.ki = parseInt($('#adv_sett_ki').val());
  to_be_saved_adv_settings.u_max = parseInt($('#adv_sett_u_max').val());
  to_be_saved_adv_settings.heater_on_min = parseInt($('#adv_sett_heater_on_min').val());
  to_be_saved_adv_settings.heater_on_max = parseInt($('#adv_sett_heater_on_max').val());
  to_be_saved_adv_settings.heater_on_off = parseInt($('#adv_sett_heater_on_off').val());
  to_be_saved_adv_settings.heater_cold = parseInt($('#adv_sett_heater_cold').val());
  to_be_saved_adv_settings.warm_up_period = parseInt($('#adv_sett_warm_up_period').val());
  to_be_saved_adv_settings.wup_heater_on = parseInt($('#adv_sett_wup_heater_on').val());
  to_be_saved_adv_settings.wup_heater_off = parseInt($('#adv_sett_wup_heater_off').val());
  return to_be_saved_adv_settings;
}

// REMOTE LOG SETTINGS

$('#collapseRemoteLogSettings').on('show.bs.collapse', function () {
  get_rl_settings(update_remoteLogSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#rl_reset').click(function () {
  get_rl_settings(update_remoteLogSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#rl_save').click(function () {
  save_rl_settings(jsonify_rl_settings,
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

// DIAGNOSTIC EVENTS
$('#collapseDiagnosticEvents').on('show.bs.collapse', function () {
  update_diag_events();
});

$('#diag_e_ack').on('click', function () {
  esp_ack_diag_events(function (xhr) {
    alert("" + xhr);
  }, function (xhr) {
    alert("" + JSON.parse(xhr.responseText).error.reason);
  });
  update_diag_events();
});

$('#diag_e_refresh').on('click', function () {
  update_diag_events();
});

function esp_ack_diag_events(success_cb, error_cb) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/diag/ack_events',
    crossDomain: esp8266.cors,
    success: function (xhr) {
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  })
}

function esp_get_diag_events(success_cb, error_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/diag/events',
    dataType: 'json',
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
}

function update_diag_events() {
  esp_get_diag_events(update_diag_events_table, function (xhr) {
    alert("" + JSON.parse(xhr.responseText).error.reason);
  });
}

function get_diag_events(success_cb, error_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/diag/events',
    dataType: 'json',
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
}

function update_diag_events_table(data) {
  $("#diag_event_table").empty();
  $("#diag_event_table").append('<thead><tr><th scope="col">Timestamp</th><th scope="col">Type</th><th scope="col">Code</th><th scope="col">Desc</th><th scope="col">Value</th></tr></thead><tbody>');
  for (var ii = 0; ii < data.diag_events.length; ii++) {
    let ts = new Date(data.diag_events[ii].ts * 1000);
    if (data.diag_events[ii].ack == 1) {
      $("#diag_event_table").append('<tr class="text-success"><td>' + ts.toString().substring(4, 24) + '</td><td>' + get_evnt_str(data.diag_events[ii].type) + '</td><td>' + String("0000" + data.diag_events[ii].code).slice(-4) + '</td><td>' + get_code_str(data.diag_events[ii].code) + '</td><td>' + data.diag_events[ii].val + '</td></tr>');
    }
    else {
      $("#diag_event_table").append('<tr><td>' + ts.toString().substring(4, 24) + '</td><td>' + get_evnt_str(data.diag_events[ii].type) + '</td><td>' + String("0000" + data.diag_events[ii].code).slice(-4) + '</td><td>' + get_code_str(data.diag_events[ii].code) + '</td><td>' + data.diag_events[ii].val + '</td></tr>');
    }
  }
  $("#diag_event_table").append('</tbody>');
}

function get_evnt_str(type) {
  var evnt_str = [];
  evnt_str[parseInt(1, 16)] = "FATAL";
  evnt_str[parseInt(2, 16)] = "ERROR";
  evnt_str[parseInt(4, 16)] = "WARN";
  evnt_str[parseInt(8, 16)] = "INFO";
  evnt_str[parseInt(10, 16)] = "DEBUG";
  evnt_str[parseInt(20, 16)] = "TRACE";
  return evnt_str[parseInt(type, 16)];
}