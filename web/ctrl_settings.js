// command.js

// spinner while awaiting for page load
$(document).ready(function () {
  esp_get_prg_list().then(function (data) {
    update_program_list(data).then(function (data) {
      update_programs(data).then(function () {
        esp_get_ctrl_settings().then(function () {
          esp_get_ctrl_adv_settings().then(function () {
            esp_get_rl_settings().then(function () {
              hide_spinner(500);
            })
          })
        })
      })
    })
  })
});

// SETTINGS

function esp_get_prg_list() {
  return esp_query({
    type: 'GET',
    url: '/api/ctrl/program',
    dataType: 'json',
    success: null,
    error: query_err
  });
}

function update_program_list(data) {
  return new Promise(function (resolve) {
    $('#sett_prg_name')
      .empty()
      .append(function () {
        var prg_count = data.prg_count;
        var options = '';
        for (var ii = 0; ii < prg_count; ii++) {
          options += '<option value="' + data.prg_headings[ii].id + '">' + data.prg_headings[ii].desc + '</option>';
        };
        return options;
      });
    resolve(data);
  });
}

function esp_get_ctrl_settings() {
  return esp_query({
    type: 'GET',
    url: '/api/ctrl/settings',
    dataType: 'json',
    success: update_settings,
    error: query_err
  });
}

function update_settings(data) {
  $('#sett_ctrl_mode').val(data.ctrl_mode);
  if (data.manual_pulse_on == 0)
    $('#sett_manual').val(0);
  else
    $('#sett_manual').val(1);
  $('#sett_man_on').val(data.manual_pulse_on);
  $('#sett_man_off').val(data.manual_pulse_off);
  $('#sett_auto_sp').val((data.auto_setpoint / 10).toFixed(1));
  $('#sett_prg_name').val(data.program_id);
  if (data.pwr_off_timer == 0)
    $('#sett_off_timer_en').val(0);
  else
    $('#sett_off_timer_en').val(1);
  $('#sett_off_timer').val(data.pwr_off_timer);
  refresh_settings_view();
}

function refresh_settings_view() {
  if ($('#sett_ctrl_mode').val() == 0) {
    $("#manual_settings").addClass('d-none');
    $("#manual_settings_timers").addClass('d-none');
    $("#sett_auto").addClass('d-none');
    $("#sett_prg").addClass('d-none');
    $("#sett_pwr_off").addClass('d-none');
  }
  if ($('#sett_ctrl_mode').val() == 1) {
    $("#manual_settings").removeClass('d-none');
    if ($('#sett_manual').val() == 0) {
      $("#manual_settings_timers").addClass('d-none');
    }
    else {
      $("#manual_settings_timers").removeClass('d-none');
    }
    $("#sett_auto").addClass('d-none');
    $("#sett_prg").addClass('d-none');
    $("#sett_pwr_off").removeClass('d-none');
    if ($("#sett_off_timer_en").val() == 0)
      $("#sett_off_timer").attr('readonly', true);
    else
      $("#sett_off_timer").attr('readonly', false);
  }
  if ($('#sett_ctrl_mode').val() == 2) {
    $("#manual_settings").addClass('d-none');
    $("#manual_settings_timers").addClass('d-none');
    $("#sett_auto").removeClass('d-none');
    $("#sett_prg").addClass('d-none');
    $("#sett_pwr_off").removeClass('d-none');
    if ($("#sett_off_timer_en").val() == 0)
      $("#sett_off_timer").attr('readonly', true);
    else
      $("#sett_off_timer").attr('readonly', false);
  }
  if ($('#sett_ctrl_mode').val() == 3) {
    $("#manual_settings").addClass('d-none');
    $("#manual_settings_timers").addClass('d-none');
    $("#sett_auto").addClass('d-none');
    $("#sett_prg").removeClass('d-none');
    $("#sett_pwr_off").addClass('d-none');
  }
}

$('#sett_off_timer').change(function () {
  $('#sett_off_timer').val(Math.floor($('#sett_off_timer').val()))
});

$('#sett_man_on').change(function () {
  $('#sett_man_on').val(Math.floor($('#sett_man_on').val()))
});

$('#sett_man_off').change(function () {
  $('#sett_man_off').val(Math.floor($('#sett_man_off').val()))
});

$('#sett_ctrl_mode').change(function () {
  refresh_settings_view();
});

$('#sett_manual').change(function () {
  refresh_settings_view();
});

$('#sett_off_timer_en').change(function () {
  refresh_settings_view();
});

$('#sett_refresh').click(function () {
  show_spinner().then(function () {
    esp_get_prg_list().then(function (data) {
      update_program_list(data).then(function () {
        esp_get_ctrl_settings().then(function () {
          hide_spinner(500)
        });
      });
    });
  });
});

function save_ctrl_settings() {
  return esp_query({
    type: 'POST',
    url: '/api/ctrl/settings',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify(ctrlSettings()),
    success: null,
    error: query_err
  });
}

function ctrlSettings() {
  var ctrl_sett = new Object;
  ctrl_sett.ctrl_mode = parseInt($('#sett_ctrl_mode').val());
  if ($('#sett_manual').val() == 0)
    ctrl_sett.manual_pulse_on = 0;
  else
    ctrl_sett.manual_pulse_on = parseInt($('#sett_man_on').val());
  ctrl_sett.manual_pulse_off = parseInt($('#sett_man_off').val());
  ctrl_sett.auto_setpoint = parseFloat($('#sett_auto_sp').val()) * 10;
  if ($('#sett_prg_name').val()) {
    ctrl_sett.program_id = parseInt($('#sett_prg_name').val());
  }
  else {
    ctrl_sett.program_id = 0;
  }
  if ($('#sett_off_timer_en').val() == 0)
    ctrl_sett.power_off_timer = 0;
  else
    ctrl_sett.power_off_timer = parseInt($('#sett_off_timer').val());
  return ctrl_sett;
}

// function jsonify_settings() {
//   var ctrl_sett = new Object;
//   ctrl_sett.ctrl_mode = parseInt($('#sett_ctrl_mode').val());
//   if ($('#sett_manual').val() == 0)
//     ctrl_sett.manual_pulse_on = 0;
//   else
//     ctrl_sett.manual_pulse_on = parseInt($('#sett_man_on').val());
//   ctrl_sett.manual_pulse_off = parseInt($('#sett_man_off').val());
//   ctrl_sett.auto_setpoint = parseFloat($('#setpoint').val()) * 10;
//   if ($('#sett_prg_name').val()) {
//     ctrl_sett.program_id = parseInt($('#sett_prg_name').val());
//   }
//   else {
//     ctrl_sett.program_id = 0;
//   }
//   if ($('#sett_off_timer_en').val() == 0)
//     ctrl_sett.pwr_off_timer = 0;
//   else
//     ctrl_sett.pwr_off_timer = parseInt($('#sett_off_timer').val());
//   return ctrl_sett;
// }
// 
$('#sett_save').click(function () {
  if (!($('#sett_prg_name').val()) && (parseInt($('#sett_ctrl_mode').val()) == 3)) {
    alert("Cannot save null program...");
    return;
  }
  show_spinner().then(function () {
    save_ctrl_settings().then(function () {
      alert("Setting saved.");
      hide_spinner(500);
    });
  });
});

// ADVANCED CONTROL SETTINGS

function esp_get_ctrl_adv_settings() {
  return esp_query({
    type: 'GET',
    url: '/api/ctrl/advSettings',
    dataType: 'json',
    success: update_advCtrlSettings,
    error: query_err
  });
}

function update_advCtrlSettings(data) {
  $('#adv_sett_kp').val(data.kp);
  $('#adv_sett_kd').val(data.kd);
  $('#adv_sett_ki').val(data.ki);
  $('#adv_sett_kd_dt').val(data.kd_dt);
  $('#adv_sett_u_max').val(data.u_max);
  $('#adv_sett_heater_on_min').val(data.heater_on_min);
  $('#adv_sett_heater_on_max').val(data.heater_on_max);
  $('#adv_sett_heater_on_off').val(data.heater_on_off);
  $('#adv_sett_heater_cold').val(data.heater_cold);
  $('#adv_sett_warm_up_period').val(data.warm_up_period);
  $('#adv_sett_wup_heater_on').val(data.wup_heater_on);
  $('#adv_sett_wup_heater_off').val(data.wup_heater_off);
}

$("#adv_sett_kd_dt").on('change', function () {
  var value = Math.floor($('#adv_sett_kd_dt').val());
  if (value < 1)
    $('#adv_sett_kd_dt').val(1);
  else if (value > 60)
    $('#adv_sett_kd_dt').val(60);
  else
    $('#adv_sett_kd_dt').val(value);
});

$('#adv_sett_refresh').click(function () {
  show_spinner()
    .then(function () {
      esp_get_ctrl_adv_settings()
        .then(function () {
          hide_spinner(500)
        });
    });
});

function jsonify_advCtrlSettings() {
  var adv_settings = new Object;
  adv_settings.kp = parseInt($('#adv_sett_kp').val());
  adv_settings.kd = parseInt($('#adv_sett_kd').val());
  adv_settings.ki = parseInt($('#adv_sett_ki').val());
  adv_settings.kd_dt = parseInt($('#adv_sett_kd_dt').val());
  adv_settings.u_max = parseInt($('#adv_sett_u_max').val());
  adv_settings.heater_on_min = parseInt($('#adv_sett_heater_on_min').val());
  adv_settings.heater_on_max = parseInt($('#adv_sett_heater_on_max').val());
  adv_settings.heater_on_off = parseInt($('#adv_sett_heater_on_off').val());
  adv_settings.heater_cold = parseInt($('#adv_sett_heater_cold').val());
  adv_settings.warm_up_period = parseInt($('#adv_sett_warm_up_period').val());
  adv_settings.wup_heater_on = parseInt($('#adv_sett_wup_heater_on').val());
  adv_settings.wup_heater_off = parseInt($('#adv_sett_wup_heater_off').val());
  return adv_settings;
}

function save_advSettings() {
  return esp_query({
    type: 'POST',
    url: '/api/ctrl/advSettings',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify(jsonify_advCtrlSettings()),
    success: null,
    error: query_err
  });
}

$('#adv_sett_save').click(function () {
  show_spinner().then(function () {
    save_advSettings().then(function () {
      alert("Setting saved.");
      hide_spinner(500);
    });
  });
});


// REMOTE LOG SETTINGS

function esp_get_rl_settings() {
  return esp_query({
    type: 'GET',
    url: '/api/ctrl/remoteLog',
    dataType: 'json',
    success: update_rLogSettings,
    error: query_err
  });
}

function update_rLogSettings(data) {
  if (data.enabled == 0)
    $('#rl_enabled').val(0);
  else
    $('#rl_enabled').val(1);
  $('#rl_host').val(data.host);
  $('#rl_port').val(data.port);
  $('#rl_path').val(data.path);
}

$('#rl_sett_refresh').click(function () {
  show_spinner().then(function () {
    esp_get_rl_settings().then(function () {
      hide_spinner(500)
    });
  });
});

function jsonify_rLogSettings() {
  var rl_settings = new Object;
  rl_settings.enabled = parseInt($('#rl_enabled').val());
  rl_settings.host = $('#rl_host').val();
  rl_settings.port = parseInt($('#rl_port').val());
  rl_settings.path = $('#rl_path').val();
  return rl_settings;
}

function save_rl_settings() {
  return esp_query({
    type: 'POST',
    url: '/api/ctrl/remoteLog',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify(jsonify_rLogSettings()),
    success: null,
    error: query_err
  });
}

$('#rl_sett_save').click(function () {
  show_spinner().then(function () {
    save_rl_settings().then(function () {
      alert("Setting saved.");
      hide_spinner(500);
    });
  });
});

// PROGRAMS

$('#programs_refresh').click(function () {
  show_spinner().then(function () {
    esp_get_prg_list().then(function (data) {
      update_programs(data).then(function () {
        hide_spinner(500);
      })
    })
  })
});

var program_id = -1;

$('#programs_new').click(function () {
  program_id = -1;
  $('#programModalTitle').text('New program');
  load_program_modal(-1);
});

var prg_headings = [];

function update_programs(data) {
  return new Promise(function (resolve) {
    prg_headings = data.prg_headings;
    $("#programs_table").empty();
    $("#programs_table").append('<thead><tr><th scope="col">Program</th><th scope="col" style="width:20%;">Actions</th></tr></thead><tbody>');
    for (var ii = 0; ii < data.prg_headings.length; ii++) {
      $("#programs_table").append('<tr><td>' +
        data.prg_headings[ii].desc +
        '</td><td><button class="btn btn-sm" onclick="modify_program(' +
        data.prg_headings[ii].id +
        ')"><i class="fa fa-pencil-square-o"></i></button><button class="btn btn-sm" onclick="delete_program(' +
        data.prg_headings[ii].id +
        ')"><i class="fa fa-trash-o"></i></button></td></tr>');
    }
    $("#programs_table").append('</tbody>');
    resolve(data);
  });
}

function get_program_name(idx) {
  for (var ii = 0; ii < prg_headings.length; ii++) {
    if (prg_headings[ii].id == idx)
      return prg_headings[ii].desc;
  }
  return "Unknown program";
}

function modify_program(id) {
  $('#programModalTitle').text(get_program_name(id));
  program_id = id;
  load_program_modal(id);
}

// PROGRAM DELETE

function esp_del_program(id) {
  return esp_query({
    type: 'DELETE',
    url: '/api/ctrl/program/' + id,
    dataType: 'json',
    success: null,
    error: query_err
  });
}

function delete_program(id) {
  if (confirm("Deleted programs cannot be recovered.\nConfirm delete..."))
    show_spinner().then(function () {
      esp_del_program(id).then(function () {
        esp_get_prg_list().then(function (data) {
          update_program_list(data).then(function (data) {
            update_programs(data).then(function () {
              esp_get_ctrl_settings().then(function () {
                hide_spinner(500);
              })
            })
          })
        })
      })
    });
}

// PROGRAM EDIT

var curr_program = new Object();

function esp_get_program(id) {
  return esp_query({
    type: 'GET',
    url: '/api/ctrl/program/' + id,
    dataType: 'json',
    success: function (data) {
      curr_program = data;
      curr_program.name = get_program_name(id);
      update_program_modal(false);
    },
    error: query_err
  });
}

function esp_create_program() {
  return esp_query({
    type: 'POST',
    url: '/api/ctrl/program',
    dataType: 'json',
    data: JSON.stringify(curr_program),
    timeout: (5000 + curr_program.periods * 100),
    success: null,
    error: query_err
  });
}

function esp_post_program() {
  show_spinner().then(function () {
    esp_create_program().then(function (data) {
      alert(data.msg);
      esp_get_prg_list().then(function (data) {
        update_program_list(data).then(function (data) {
          update_programs(data).then(function () {
            esp_get_ctrl_settings().then(function () {
              $('#programModal').modal('hide');
              hide_spinner(500);
            })
          })
        })
      })
    })
  });
}

function esp_modify_program(id) {
  return esp_query({
    type: 'PUT',
    url: '/api/ctrl/program/' + id,
    dataType: 'json',
    data: JSON.stringify(curr_program),
    timeout: (5000 + curr_program.periods * 100),
    success: null,
    error: query_err
  });
}

function esp_put_program(id) {
  show_spinner().then(function () {
    esp_modify_program(id).then(function (data) {
      alert(data.msg);
      esp_get_prg_list().then(function (data) {
        update_program_list(data).then(function (data) {
          update_programs(data).then(function () {
            esp_get_ctrl_settings().then(function () {
              hide_spinner(500);
            })
          })
        })
      })
    })
  });
}

function update_program_modal(blank_prg) {
  if (blank_prg) {
    $("#program_name").val("");
    $("#program_def_sp").val((0).toFixed(1));
  }
  else {
    $("#program_name").val(curr_program.name);
    $("#program_def_sp").val((curr_program.min_temp / 10).toFixed(1));
  }

  $("#periods_table").empty();
  $("#periods_table").append('<thead><tr class="text-center"><th scope="col"> </th><th scope="col">Day</th><th scope="col">Start</th><th scope="col">End</th><th scope="col">Setpoint</th><th scope="col"></th></tr></thead><tbody>');
  var period = 0;
  if (!blank_prg) {
    while (period < curr_program.periods.length) {
      $("#periods_table").append('<tr><td><button type="button-sm" class="btn btn-default" onclick="add_prg_period(' +
        period +
        ');"><i class="fa fa-plus-circle" aria-hidden="true"></i></button></td><td><select class="custom-select-sm" id="wk_day_' +
        period +
        '" onchange="update_wd_period(' +
        period +
        ');"><option value="0"> <option value="1">Mon</option><option value="2">Tue</option><option value="3">Wed</option><option value="4">Thu</option><option value="5">Fri</option><option value="6">Sat</option><option value="7">Sun</option><option value="8">All</option></select></td><td><input id="start_' +
        period +
        '" value="" class="form-control-sm text-center" style="width:5rem"data-default="00:00" onmousedown="$(\x27#start_' +
        period +
        '\x27).clockpicker({autoclose: true});" onchange="update_start_period(' +
        period +
        '); " readonly></td><td><input id="end_' +
        period +
        '" value="" class="form-control-sm text-center" style="width: 5rem" data-default="00:00" onmousedown="$(\x27#end_' +
        period +
        '\x27).clockpicker({autoclose: true});" onchange = "update_end_period(' +
        period +
        ');" readonly></td><td><input type="number" min="0" max="99" step="0.1" placeholder="°C" class="form-control-sm  text-center" id="sp_' +
        period +
        '" onchange="update_sp_period(' +
        period +
        ');"></td><td><button type="button-sm" class="btn btn-default" onclick="del_prg_period(' +
        period +
        ');"><i class="fa fa-trash" aria-hidden="true"></i></button></td></tr>');
      period += 1;
    }
    period = 0;
    while (period < curr_program.periods.length) {
      var el_name = '#wk_day_' + period;
      if (curr_program.periods[period].wd)
        $(el_name).val(curr_program.periods[period].wd);
      var start = "#start_" + period;
      if (curr_program.periods[period].b)
        $(start).val(("00" + Math.floor(curr_program.periods[period].b / 60)).slice(-2) + ":" + ("00" + (curr_program.periods[period].b % 60)).slice(-2));
      var end = "#end_" + period;
      if (curr_program.periods[period].e)
        $(end).val(("00" + Math.floor(curr_program.periods[period].e / 60)).slice(-2) + ":" + ("00" + (curr_program.periods[period].e % 60)).slice(-2));
      var sp = '#sp_' + period;
      $(sp).val((curr_program.periods[period].sp / 10).toFixed(1));
      period += 1;
    }
  }
  $("#periods_table").append('<tr><td><button type="button-sm" class="btn btn-default" onclick="add_prg_period(' + period + ')"><i class="fa fa-plus-circle" aria-hidden="true"></i></button></td><td></td><td></td><td></td><td></td><td></td></tr></tbody>');
  $('#programModal').modal('show');
}

function load_program_modal(id) {
  if (id >= 0)
    esp_get_program(id);
  else {
    curr_program = {
      "id": -1,
      "name": "",
      "min_temp": 0,
      "period_count": 0,
      "periods": []
    };
    update_program_modal(true);
  }
}

$("#program_name").on('change', function () {
  curr_program.name = $("#program_name").val();
});

$("#program_def_sp").on('change', function () {
  var value = Math.floor($('#program_def_sp').val() * 10);
  curr_program.min_temp = value;
  $('#program_def_sp').val((value / 10).toFixed(1));
});

$('#off_timer').change(function () {
  $('#off_timer').val(Math.floor($('#off_timer').val()))
});

function add_prg_period(idx) {
  var new_prg_periods = {
    "id": curr_program.id,
    "name": curr_program.name,
    "min_temp": curr_program.min_temp,
    "period_count": (curr_program.period_count + 1),
    "periods": []
  };
  var new_period = new Object();
  for (var ii = 0; ii < idx; ii++) {
    new_prg_periods.periods[ii] = curr_program.periods[ii];
  }
  new_prg_periods.periods.push(new_period);
  for (var ii = idx; ii < (curr_program.periods.length); ii++) {
    new_prg_periods.periods[ii + 1] = curr_program.periods[ii];
  }
  curr_program = new_prg_periods;
  update_program_modal(false);
}

function update_wd_period(idx) {
  var el_name = '#wk_day_' + idx;
  curr_program.periods[idx].wd = parseInt($(el_name).val());
}

function update_start_period(idx) {
  var start = "#start_" + idx;
  curr_program.periods[idx].b = parseInt(($(start).val()).slice(0, 2)) * 60 + parseInt(($(start).val()).slice(-2));
}

function update_end_period(idx) {
  var end = "#end_" + idx;
  curr_program.periods[idx].e = parseInt(($(end).val()).slice(0, 2)) * 60 + parseInt(($(end).val()).slice(-2));
}

function update_sp_period(idx) {
  var el_name = '#sp_' + idx;
  $(el_name).val(((Math.floor($(el_name).val() * 10)) / 10).toFixed(1));
  curr_program.periods[idx].sp = $(el_name).val() * 10;
}

function del_prg_period(idx) {
  var new_prg_periods = {
    "id": curr_program.id,
    "name": curr_program.name,
    "min_temp": curr_program.min_temp,
    "period_count": (curr_program.period_count - 1),
    "periods": []
  };
  for (var ii = 0; ii < idx; ii++) {
    new_prg_periods.periods[ii] = curr_program.periods[ii];
  }
  for (var ii = (idx + 1); ii < (curr_program.periods.length); ii++) {
    new_prg_periods.periods[ii - 1] = curr_program.periods[ii];
  }
  curr_program = new_prg_periods;
  update_program_modal(false);
}

$("#commandModalReset").on("click", function () {
  load_program_modal(program_id);
});

$("#commandModalSave").on("click", function () {
  // check for empty field
  var empty_fields_exist = false;
  if (curr_program.name == "")
    empty_fields_exist = true;
  for (var ii = 0; ii < curr_program.periods.length; ii++) {
    if (!('wd' in curr_program.periods[ii]) || (curr_program.periods[ii].wd == null))
      empty_fields_exist = true;
    if (!('b' in curr_program.periods[ii]) || (curr_program.periods[ii].b == null))
      empty_fields_exist = true;
    if (!('e' in curr_program.periods[ii]) || (curr_program.periods[ii].e == null))
      empty_fields_exist = true;
    if (!('sp' in curr_program.periods[ii]) || (curr_program.periods[ii].sp == null))
      empty_fields_exist = true;
  }
  if (empty_fields_exist) {
    alert("There are empty fields...");
    return;
  }
  // no empty fields
  // curr_program.name = $("#program_name").val();
  if (program_id == -1) {
    // it's a new program
    // check if program name already exists...
    var name_already_in_use = false;
    var ii;
    for (ii = 0; ii < prg_headings.length; ii++) {
      if (prg_headings[ii].desc == $("#program_name").val())
        name_already_in_use = true;
    }
    if (name_already_in_use) {
      alert("Program name already in use...");
      return;
    }
    else
      esp_post_program();
  } else {
    // it's a modified program
    // check if program name already exists...
    var name_already_in_use = false;
    $('#program_list option').each(function (index, element) {
      if ((element.text == $("#sel_prg_name").val()) && (element.value != curr_program.id)) {
        name_already_in_use = true;
      }
    });
    if (name_already_in_use) {
      alert("Program name already in use...");
      return;
    }
    else
      esp_put_program(program_id);
  }
});
