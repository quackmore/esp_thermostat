const esp8266 = {
  "url": "http://192.168.1.105",
  // "url": "http://192.168.1.185",
  // "url": "",
  // "cors": false
  "cors": true
};

function esp_get_current_vars(success_cb, error_cb) {
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

function esp_get_info(success_cb, error_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/info',
    dataType: 'json',
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
  // {
  //   "app_name": "THERMOSTAT",
  //   "app_version": "v0.2-20-g8cbd71e",
  //   "espbot_name": "temp_logger-1",
  //   "espbot_version": "v2.0-4-g085b612",
  //   "library_version": "v2.0-1-g1598749",
  //   "chip_id": "5169284",
  //   "sdk_version": "3.1.0-dev(b1f42da)",
  //   "boot_version": "7"
  // }
}

function esp_get_program_list(success_cb, error_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/program',
    dataType: 'json',
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
  // {
  //     "prgm_count": 3,
  //     "headings": [
  //         {"id": 1,"desc":"first"},
  //         {"id": 2,"desc":"second"},
  //         {"id": 3,"desc":"third"}
  //     ]
  // }
}

function esp_get_program(prg_id, success_cb, error_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/program/' + prg_id,
    dataType: 'json',
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
  // PROGRAM
  // {
  //     "id": 1,
  //     "min_temp": 100,
  //     "period_count": 3,
  //     "periods": [
  //         {"wd": 8,"b":1880,"e":1880,"sp":200},
  //         {"wd": 8,"b":1880,"e":1880,"sp":200},
  //         {"wd": 8,"b":1880,"e":1880,"sp":200}
  //     ]
  // }
}

function esp_del_program(prg_id, success_cb, error_cb) {
  $.ajax({
    type: 'DELETE',
    url: esp8266.url + '/api/program/' + prg_id,
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
}

function esp_put_program(program_obj, success_cb, error_cb) {
  $.ajax({
    type: 'PUT',
    url: esp8266.url + '/api/program/' + program_obj.id,
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify(program_obj),
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
  // PROGRAM
  // {
  //     "name": name,
  //     "id": 1,
  //     "min_temp": 100,
  //     "period_count": 3,
  //     "periods": [
  //         {"wd": 8,"b":1880,"e":1880,"sp":200},
  //         {"wd": 8,"b":1880,"e":1880,"sp":200},
  //         {"wd": 8,"b":1880,"e":1880,"sp":200}
  //     ]
  // }
}

function esp_post_program(program_obj, success_cb, error_cb) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/program',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify(program_obj),
    crossDomain: esp8266.cors,
    success: function (data) {
      success_cb(data);
    },
    error: function (xhr) {
      error_cb(xhr);
    }
  });
  // PROGRAM
  // {
  //     "name": name,
  //     "id": 1,
  //     "min_temp": 100,
  //     "period_count": 3,
  //     "periods": [
  //         {"wd": 8,"b":1880,"e":1880,"sp":200},
  //         {"wd": 8,"b":1880,"e":1880,"sp":200},
  //         {"wd": 8,"b":1880,"e":1880,"sp":200}
  //     ]
  // }
}

function esp_get_settings(success_cb, error_cb) {
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

function esp_save_settings(get_data, success_cb, error_cb) {
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

function esp_get_rl_settings(success_cb, error_cb) {
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

function esp_save_rl_settings(get_data, success_cb, error_cb) {
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

function esp_get_adv_ctrl_settings(success_cb, error_cb) {
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

function esp_save_adv_ctrl_settings(get_data, success_cb, error_cb) {
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
  setTimeout(function () {
    $('.modal').modal('hide');
  }, 500);
  update_collapseCurrent();
  update_version();
});

$('#collapseCurrent').on('show.bs.collapse', function () {
  update_collapseCurrent();
});

function update_version() {
  esp_get_info(function (data) {
    $("#app_version").text("version " + data.app_version);
  }, function (xhr) {
    alert("" + xhr.responseText);
  });
}

function update_collapseCurrent() {
  esp_get_current_vars(update_current_vars, function (xhr) {
    alert("" + xhr.responseText);
  });
}

function update_current_vars(data) {
  var week_day = new Array("Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun");
  $("#thermo_date").text(function () {
    var date = new Date(data.ctrl_date * 1000);
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
    if ((data.ctrl_mode == 2) || (data.ctrl_mode == 3))
      return "setpoint " + (data.auto_setpoint / 10).toFixed(1);
    else
      return "no setpoint";
  });
  var mode = new Array("OFF", "MANUAL", "AUTO", "PROGRAM");
  $("#thermo_mode").text(function () {
    if (data.ctrl_mode <= 2)
      return "mode " + mode[data.ctrl_mode];
    else
      return mode[data.ctrl_mode] + " " + data.program_name;
  });
  if (data.pwr_off_timer == 0) {
    $("#thermo_pwr_off").addClass('d-none');
  } else {
    $("#thermo_pwr_off").removeClass('d-none');
    var now = new Date();
    var timer_started = new Date(data.pwr_off_timer_started_on * 1000);
    var pwr_off_minutes = Math.floor((now - timer_started) / 60000);
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
  esp_save_settings(jsonify_settings,
    function (data) {
      alert(data.msg);
    }, function (xhr) {
      alert("" + xhr.responseText);
    });
});

function update_collapseSettings() {
  esp_get_program_list(update_programs, function (xhr) {
    alert("" + xhr.responseText);
  });
}

function update_programs(data) {
  $('#program_name')
    .empty()
    .append(function () {
      var prg_count = data.prgm_count;
      var options = '';
      for (var ii = 0; ii < prg_count; ii++) {
        options += '<option value="' + data.headings[ii].id + '">' + data.headings[ii].desc + '</option>';
      };
      return options;
    });
  esp_get_settings(update_settings, function (xhr) {
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
  $('#program_name').val(data.program_id);
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
    $("#program_select").addClass('d-none');
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
    $("#program_select").addClass('d-none');
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
    $("#program_select").addClass('d-none');
    $("#power_off_timer").removeClass('d-none');
    if ($("#off_timer_en").val() == 0)
      $("#off_timer").attr('readonly', true);
    else
      $("#off_timer").attr('readonly', false);
  }
  if ($('#ctrl_mode').val() == 3) {
    $("#manual_settings").addClass('d-none');
    $("#manual_pulse_on").addClass('d-none');
    $("#manual_pulse_off").addClass('d-none');
    $("#auto_setpoint").addClass('d-none');
    $("#program_select").removeClass('d-none');
    $("#power_off_timer").addClass('d-none');
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
  to_be_saved_settings.program_id = parseInt($('#program_name').val());
  if ($('#off_timer_en').val() == 0)
    to_be_saved_settings.power_off_timer = 0;
  else
    to_be_saved_settings.power_off_timer = parseInt($('#off_timer').val());
  return to_be_saved_settings;
}

// PROGRAMS

$('#collapsePrograms').on('show.bs.collapse', function () {
  update_collapsePrograms();
});

function update_collapsePrograms() {
  esp_get_program_list(update_program_list, function (xhr) {
    alert("" + xhr.responseText);
  });
}

function update_program_list(data) {
  $('#program_list')
    .empty()
    .append(function () {
      var prg_count = data.prgm_count;
      var options = '';
      for (var ii = 0; ii < prg_count; ii++) {
        options += '<option value="' + data.headings[ii].id + '">' + data.headings[ii].desc + '</option>';
      };
      return options;
    });
  $("#prg_select_row").removeClass('d-none');
  $("#prg_rename_row").addClass('d-none');
  if (data.prgm_count > 0) {
    var selected = $('#program_name option:selected').val();
    if (!selected) {
      // selected is undefined
      selected = data.headings[0].id;
    }
    else {
      // check that selected is into the headings
      var selected_found = false;
      for (var ii = 0; ii < data.headings.length; ii++) {
        if (selected == data.headings[ii].id) { 
          selected_found = true; 
          break;
        }
      }
      if (!selected_found)
        selected = data.headings[0].id;
    }
    $('#program_list').val(selected);
    $('#sel_prg_name').val(function () {
      for (var ii = 0; ii < data.headings.length; ii++) {
        if (data.headings[ii].id == selected) {
          return data.headings[ii].desc;
        }
      }
      return "";
    });
    esp_get_program(selected, update_prg_periods, function (xhr) {
      alert("" + xhr.responseText);
    });
  }
}

$('#program_list').change(function () {
  esp_get_program($('#program_list').val(), update_prg_periods, function (xhr) {
    alert("" + xhr.responseText);
  });
  $('#sel_prg_name').val($('#program_list option:selected').text());
});

var cur_prg_periods = new Object();

function update_prg_periods(data) {
  cur_prg_periods = data;
  update_prg_form();
}

function update_prg_form() {
  // form html first
  var form_content = "";
  form_content += '<div class="form-group row"><label for="prg_def_sp" class="col-sm-6 col-form-label text-left">Default setpoint</label><div class="input-group col-sm-6">  <input type="number" min="0" max="99" step="0.1" placeholder="°C" class="form-control" id="prg_def_sp" onchange="update_def_sp();"></div></div>';
  for (var line = 0; line < cur_prg_periods.periods.length; line++) {
    form_content += '<div class="form-row"><div class="col mx-0 px-0"><button type="button" class="btn btn-default" onclick="add_prg_period(';
    form_content += line;
    form_content += ');"><i class="fa fa-plus-circle" aria-hidden="true"></i></button></div><div class="col-xs-2 ml-0 pl-0 mr-2 pr-0"><select class="custom-select" id="wk_day_';
    form_content += line;
    form_content += '" onchange="update_wd_period(';
    form_content += line;
    form_content += ');"><option value="" disabled selected>day</option><option value="1">Mon</option><option value="2">Tue</option><option value="3">Wed</option><option value="4">Thu</option><option value="5">Fri</option><option value="6">Sat</option><option value="7">Sun</option><option value="8">All</option></select></div><div class="col-xs-1 mx-0 px-0"><input type="number" min="0" max="24" step="1" placeholder="hh" class="form-control" id="start_hh_';
    form_content += line;
    form_content += '" onchange="update_b_period(';
    form_content += line;
    form_content += ');"></div><div class="col-xs-1 ml-0 mr-2 px-0"><input type="number" min="0" max="59" step="1" placeholder="mm" class="form-control" id="start_mm_';
    form_content += line;
    form_content += '" onchange="update_b_period(';
    form_content += line;
    form_content += ');"></div><div class="col-xs-1 mx-0 px-0"><input type="number" min="0" max="24" step="1" placeholder="hh" class="form-control" id="end_hh_';
    form_content += line;
    form_content += '" onchange="update_e_period(';
    form_content += line;
    form_content += ');"></div><div class="col-xs-1 ml-0 mr-2 px-0"><input type="number" min="0" max="59" step="1" placeholder="mm" class="form-control" id="end_mm_';
    form_content += line;
    form_content += '" onchange="update_e_period(';
    form_content += line;
    form_content += ');"></div><div class="col-xs-1 mx-0 px-0"><input type="number" min="0" max="99" step="0.1" placeholder="°C" class="form-control" id="sp_';
    form_content += line;
    form_content += '" onchange="update_sp_period(';
    form_content += line;
    form_content += ');"></div><div class="col mx-0 px-0">  <button type="button" class="btn btn-default" onclick="del_prg_period(';
    form_content += line;
    form_content += ');"><i class="fa fa-trash" aria-hidden="true"></i></button></div></div>';
  }
  form_content += '<div class="form-row"><div class="col mx-0 px-0"><button type="button" class="btn btn-default" onclick="add_prg_period(';
  form_content += cur_prg_periods.periods.length;
  form_content += ');"><i class="fa fa-plus-circle" aria-hidden="true"></i></button></div></div>';
  $('#prg_periods').html(form_content);
  // now form data
  $('#prg_def_sp').val((cur_prg_periods.min_temp / 10).toFixed(1));
  var el_name;
  for (var line = 0; line < cur_prg_periods.periods.length; line++) {
    el_name = '#wk_day_' + line;
    $(el_name).val(cur_prg_periods.periods[line].wd);
    el_name = '#start_hh_' + line;
    $(el_name).val(Math.floor(cur_prg_periods.periods[line].b / 60));
    el_name = '#start_mm_' + line;
    $(el_name).val((cur_prg_periods.periods[line].b) % 60);
    el_name = '#end_hh_' + line;
    $(el_name).val(Math.floor(cur_prg_periods.periods[line].e / 60));
    el_name = '#end_mm_' + line;
    $(el_name).val((cur_prg_periods.periods[line].e) % 60);
    el_name = '#sp_' + line;
    $(el_name).val((cur_prg_periods.periods[line].sp / 10).toFixed(1));
  }
}

function add_prg_period(idx) {
  var new_prg_periods = {
    "id": cur_prg_periods.id,
    "min_temp": cur_prg_periods.min_temp,
    "period_count": (cur_prg_periods.period_count + 1),
    "periods": []
  };
  var new_period = new Object();
  for (var ii = 0; ii < idx; ii++) {
    new_prg_periods.periods[ii] = cur_prg_periods.periods[ii];
  }
  new_prg_periods.periods.push(new_period);
  for (var ii = idx; ii < (cur_prg_periods.periods.length); ii++) {
    new_prg_periods.periods[ii + 1] = cur_prg_periods.periods[ii];
  }
  cur_prg_periods = new_prg_periods;
  update_prg_form();
}

function update_wd_period(idx) {
  var el_name = '#wk_day_' + idx;
  cur_prg_periods.periods[idx].wd = parseInt($(el_name).val());
}

function update_b_period(idx) {
  var start_hh = '#start_hh_' + idx;
  var start_mm = '#start_mm_' + idx;
  $(start_hh).val(Math.floor($(start_hh).val()));
  $(start_mm).val(Math.floor($(start_mm).val()));
  cur_prg_periods.periods[idx].b = parseInt($(start_hh).val() * 60) + parseInt($(start_mm).val());
}

function update_e_period(idx) {
  var end_hh = '#end_hh_' + idx;
  var end_mm = '#end_mm_' + idx;
  $(end_hh).val(Math.floor($(end_hh).val()));
  $(end_mm).val(Math.floor($(end_mm).val()));
  cur_prg_periods.periods[idx].e = parseInt($(end_hh).val() * 60) + parseInt($(end_mm).val());
}

function update_sp_period(idx) {
  var el_name = '#sp_' + idx;
  $(el_name).val(((Math.floor($(el_name).val() * 10)) / 10).toFixed(1));
  cur_prg_periods.periods[idx].sp = $(el_name).val() * 10;
}

function update_def_sp() {
  $('#prg_def_sp').val(((Math.floor($('#prg_def_sp').val() * 10)) / 10).toFixed(1));
  cur_prg_periods.min_temp = $('#prg_def_sp').val() * 10;
}

function del_prg_period(idx) {
  var new_prg_periods = {
    "id": cur_prg_periods.id,
    "min_temp": cur_prg_periods.min_temp,
    "period_count": (cur_prg_periods.period_count - 1),
    "periods": []
  };
  for (var ii = 0; ii < idx; ii++) {
    new_prg_periods.periods[ii] = cur_prg_periods.periods[ii];
  }
  for (var ii = (idx + 1); ii < (cur_prg_periods.periods.length); ii++) {
    new_prg_periods.periods[ii - 1] = cur_prg_periods.periods[ii];
  }
  cur_prg_periods = new_prg_periods;
  update_prg_form();
}

$('#programs_rename').click(function () {
  $("#prg_rename_row").removeClass('d-none');
});

$('#programs_reset').click(function () {
  update_collapsePrograms();
});

$('#programs_new').click(function () {
  $("#prg_select_row").addClass('d-none');
  $("#sel_prg_name").val("");
  $("#prg_rename_row").removeClass('d-none');
  var new_prg_periods = {
    "id": 0,
    "min_temp": NaN,
    "period_count": 1,
    "periods": []
  };
  var new_period = new Object();
  new_prg_periods.periods.push(new_period);
  cur_prg_periods = new_prg_periods;
  update_prg_form();
});

$('#programs_delete').click(function () {
  esp_del_program($('#program_list option:selected').val(),
    function (data) {
      alert(data.msg);
      update_collapsePrograms();
    }, function (xhr) {
      alert("" + xhr.responseText);
    });
});

$('#programs_save').click(function () {
  // check for empty field
  var empty_fields_exist = false;
  for (var ii = 0; ii < cur_prg_periods.periods.length; ii++) {
    if (!('wd' in cur_prg_periods.periods[ii]) || (cur_prg_periods.periods[ii].wd == null))
      empty_fields_exist = true;
    if (!('b' in cur_prg_periods.periods[ii]) || (cur_prg_periods.periods[ii].b == null))
      empty_fields_exist = true;
    if (!('e' in cur_prg_periods.periods[ii]) || (cur_prg_periods.periods[ii].e == null))
      empty_fields_exist = true;
    if (!('sp' in cur_prg_periods.periods[ii]) || (cur_prg_periods.periods[ii].sp == null))
      empty_fields_exist = true;
  }
  if (empty_fields_exist) {
    alert("There are empty fields...")
    return;
  }
  cur_prg_periods.name = $("#sel_prg_name").val();
  if ($("#prg_select_row").hasClass('d-none')) {
    // it's a new program
    // check if program name already exists...
    var name_already_in_use = false;
    $('#program_list option').each(function (index, element) {
      if (element.text == $("#sel_prg_name").val()) {
        name_already_in_use = true;
      }
    });
    if (name_already_in_use) {
      alert("Program name already in use...");
      return;
    }
    else {
      esp_post_program(cur_prg_periods,
        function (data) {
          alert(data.msg);
          update_collapsePrograms();
        }, function (xhr) {
          alert("" + xhr.responseText);
        });
    }
  } else {
    // it's a modified program
    // check if program name already exists...
    var name_already_in_use = false;
    $('#program_list option').each(function (index, element) {
      if ((element.text == $("#sel_prg_name").val()) && (element.value != cur_prg_periods.id)) {
        name_already_in_use = true;
      }
    });
    if (name_already_in_use) {
      alert("Program name already in use...");
      return;
    }
    else {
      esp_put_program(cur_prg_periods,
        function (data) {
          alert(data.msg);
          update_collapsePrograms();
        }, function (xhr) {
          alert("" + xhr.responseText);
        });
    }
  }
});

// ADVANCED CONTROL SETTINGS

$('#collapseAdvSettings').on('show.bs.collapse', function () {
  esp_get_adv_ctrl_settings(update_advCtrlSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#adv_settings_reset').click(function () {
  esp_get_adv_ctrl_settings(update_advCtrlSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#adv_settings_save').click(function () {
  esp_save_adv_ctrl_settings(jsonify_advCtrlSettings,
    function (data) {
      alert(data.msg);
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
  esp_get_rl_settings(update_remoteLogSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#rl_reset').click(function () {
  esp_get_rl_settings(update_remoteLogSettings, function (xhr) {
    alert("" + xhr.responseText);
  });
});

$('#rl_save').click(function () {
  esp_save_rl_settings(jsonify_rl_settings,
    function (data) {
      alert(data.msg);
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
  esp_ack_diag_events(function (data) {
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
    success: function (data) {
      success_cb(data);
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
    var ts = new Date(data.diag_events[ii].ts * 1000);
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