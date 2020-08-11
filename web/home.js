// command.js

// spinner while awaiting for page load
$(document).ready(function () {
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_page();
});

function update_page() {
  update_summary();
}

// Summary

function esp_get_ctrl_vars(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/ctrl/vars',
    dataType: 'json',
    crossDomain: esp8266.cors,
    //success: success_cb(data),
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_summary() {
  esp_get_ctrl_vars(function (data) {
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
  });
  setTimeout(function () {
    update_summary();
  }, 15000);
}