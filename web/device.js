// device.js

$(document).ready(function () {
  esp_get_info().then(function () {
    esp_get_wifi_info().then(function () {
      esp_get_ap().then(function () {
        esp_get_cron().then(function () {
          esp_get_mdns().then(function () {
            esp_get_datetime().then(function (data) {
              update_datetime(data).then(function () {
                esp_get_ota().then(function () {
                  esp_get_diag().then(function () {
                    hide_spinner(500);
                    setTimeout(function () {
                      periodically_update_datetime();
                    }, 10000);
                  })
                })
              })
            })
          })
        })
      })
    })
  });
});

// device info

function esp_get_info() {
  return esp_query({
    type: 'GET',
    url: '/api/info',
    dataType: 'json',
    success: update_device_info,
    error: query_err
  });
}

function update_device_info(data) {
  $("#app_name").val(data.app_name);
  $("#app_version").val(data.app_version);
  $("#dev_name").val(data.device_name);
  $("#espbot_version").val(data.espbot_version);
  $("#api_version").val(data.api_version);
  $("#library_version").val(data.library_version);
  $("#chip_id").val(data.chip_id);
  $("#sdk_version").val(data.sdk_version);
  $("#boot_version").val(data.boot_version);
  if ($('#info_buttons').hasClass("d-none")) {
    $("#dev_name").prop("disabled", true);
  }
}

$('#info_edit').on('click', function () {
  if ($('#info_buttons').hasClass("d-none")) {
    $('#info_buttons').removeClass("d-none");
    $('#dev_name').removeClass("border-0");
    $("#dev_name").prop("disabled", false);
  }
  else {
    $('#info_buttons').addClass("d-none");
    $('#dev_name').addClass("border-0");
    $("#dev_name").prop("disabled", true);
  }
});

$('#info_refresh').on('click', function () {
  show_spinner()
    .then(function () {
      esp_get_info()
        .then(function () {
          hide_spinner(500)
        });
    });
});

$('#info_save').on('click', function () {
  show_spinner();
  return esp_query({
    type: 'POST',
    url: '/api/deviceName',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ device_name: $('#dev_name').val() }),
    success: function () {
      alert("Device name saved.");
      esp_get_info()
        .then(function () {
          hide_spinner(500)
        });
    },
    error: query_err
  });
});

// wifi

function esp_get_wifi_info() {
  return esp_query({
    type: 'GET',
    url: '/api/wifi',
    dataType: 'json',
    success: update_wifi,
    error: query_err
  });
}

function update_wifi(data) {
  $("#wifi_working_as").val(data.op_mode);
  $("#wifi_ssid_label").text(function () {
    if (data.op_mode === "STATION")
      return "Connected to";
    else
      return "SSID";
  });
  $("#wifi_ssid").val(data.SSID);
  $("#wifi_ip").val(data.ip_address);
}

$('#wifi_edit').on('click', function () {
  if ($('#wifi_buttons').hasClass("d-none")) {
    $('#wifi_buttons').removeClass("d-none");
    show_spinner();
    esp_wifi_scan()
      .then((function (data) {
        return Promise.resolve(update_wifi_aps(data));
      }))
      .then(function () {
        hide_spinner(500);
      });
  }
  else {
    $('#wifi_buttons').addClass("d-none");
  }
});

function esp_wifi_scan() {
  return esp_query({
    type: 'GET',
    url: '/api/wifi/scan',
    dataType: 'json',
    timeout: 12000,
    success: null,
    error: query_err
  });
}

function update_wifi_aps(data) {
  $("#wifi_aps").empty();
  for (var ii = 0; ii < data.AP_SSIDs.length; ii++) {
    if ($("#wifi_ssid").val() === data.AP_SSIDs[ii]) {
      $("#wifi_aps").append("<option value='" + ii + "' selected>" + data.AP_SSIDs[ii] + "</option>");
    }
    else {
      $("#wifi_aps").append("<option value='" + ii + "'>" + data.AP_SSIDs[ii] + "</option>");
    }
  };
}

$('#st_pwd_view').on('click', function () {
  if ($('#wifi_pwd').attr('type') === 'password') {
    $('#wifi_pwd').attr('type', 'text');
  } else {
    $('#wifi_pwd').attr('type', 'password');
  }
});

$('#wifi_refresh').on('click', function () {
  show_spinner();
  esp_wifi_scan()
    .then(function (data) {
      return Promise.resolve(update_wifi_aps(data));
    })
    .then(function () {
      hide_spinner(500);
    });
});

$('#wifi_connect').on('click', function () {
  return esp_query({
    type: 'POST',
    url: '/api/wifi/station/cfg',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ station_ssid: $('#wifi_aps option:selected').text(), station_pwd: $('#wifi_pwd').val() }),
    success: function () {
      alert("Connecting to AP " + $('#wifi_aps option:selected').text() + " ...");
    },
    error: query_err
  });
});

// AP

function esp_get_ap() {
  return esp_query({
    type: 'GET',
    url: '/api/wifi/ap/cfg',
    dataType: 'json',
    success: update_ap,
    error: query_err
  });
}

function update_ap(data) {
  $('#ap_ch').val(data.ap_channel);
  $('#ap_pwd').val(data.ap_pwd);
  if ($('#ap_pwd').val().length < 8)
    $("#ap_save").prop("disabled", true);
  else
    $("#ap_save").prop("disabled", false);
  if ($('#ap_buttons').hasClass("d-none")) {
    $("#ap_ch").prop("disabled", true);
    $("#ap_pwd").prop("disabled", true);
  }
}

$('#ap_edit').on('click', function () {
  if ($('#ap_buttons').hasClass("d-none")) {
    $('#ap_buttons').removeClass("d-none");
    $('#ap_ch').removeClass("border-0");
    $("#ap_ch").prop("disabled", false);
    $('#ap_pwd').removeClass("border-0");
    $("#ap_pwd").prop("disabled", false);
  } else {
    $('#ap_buttons').addClass("d-none");
    $('#ap_ch').addClass("border-0");
    $("#ap_ch").prop("disabled", true);
    $('#ap_pwd').addClass("border-0");
    $("#ap_pwd").prop("disabled", true);
  }
});

$('#ap_pwd_view').on('click', function () {
  if ($('#ap_pwd').attr('type') === 'password') {
    $('#ap_pwd').attr('type', 'text');
  } else {
    $('#ap_pwd').attr('type', 'password');
  }
});

$('#ap_refresh').on('click', function () {
  show_spinner().then(function () {
    esp_get_ap()
      .then(function () {
        hide_spinner(500)
      });
  });
});

$('#ap_ch').on('change', function () {
  $('#ap_ch').val(Math.trunc($('#ap_ch').val()));
  if ($('#ap_ch').val() < 1)
    $('#ap_ch').val(1);
  if ($('#ap_ch').val() > 11)
    $('#ap_ch').val(11);
});

$('#ap_pwd').on('change', function () {
  if ($('#ap_pwd').val().length < 8)
    $("#ap_save").prop("disabled", true);
  else
    $("#ap_save").prop("disabled", false);
});

$('#ap_save').on('click', function () {
  show_spinner();
  return esp_query({
    type: 'POST',
    url: '/api/wifi/ap/cfg',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ ap_channel: Number($('#ap_ch').val()), ap_pwd: $('#ap_pwd').val() }),
    success: function () {
      alert("AP cfg saved.\nWarning: modifying the current network configuration may cause temporary disconnections...");
      hide_spinner(500);
    },
    error: query_err
  });
});

// cron

function esp_get_cron() {
  return esp_query({
    type: 'GET',
    url: '/api/cron',
    dataType: 'json',
    success: update_cron,
    error: query_err
  });
}

function update_cron(data) {
  $('#cron_status').val(data.cron_enabled);
  if ($('#cron_buttons').hasClass("d-none")) {
    $("#cron_status").prop("disabled", true);
  }
}

$('#cron_edit').on('click', function () {
  if ($('#cron_buttons').hasClass("d-none")) {
    $('#cron_buttons').removeClass("d-none");
    $('#cron_status').removeClass("border-0");
    $("#cron_status").prop("disabled", false);
  } else {
    $('#cron_buttons').addClass("d-none");
    $('#cron_status').addClass("border-0");
    $("#cron_status").prop("disabled", true);
  }
});

$('#cron_refresh').on('click', function () {
  show_spinner().then(function () {
    esp_get_cron()
      .then(function () {
        hide_spinner(500);
      });
  });
});

$('#cron_save').on('click', function () {
  show_spinner();
  return esp_query({
    type: 'POST',
    url: '/api/cron',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ cron_enabled: Number($('#cron_status').val()) }),
    success: function () {
      alert("Cron status saved.");
      esp_get_cron()
        .then(function () {
          hide_spinner(500);
        });
    },
    error: query_err
  });
});

// mDNS

function esp_get_mdns() {
  return esp_query({
    type: 'GET',
    url: '/api/mdns',
    dataType: 'json',
    success: update_mdns,
    error: query_err
  });
}

function update_mdns(data) {
  $('#mdns_status').val(data.mdns_enabled);
  if ($('#mdns_buttons').hasClass("d-none")) {
    $("#mdns_status").prop("disabled", true);
  }
}

$('#mdns_edit').on('click', function () {
  if ($('#mdns_buttons').hasClass("d-none")) {
    $('#mdns_buttons').removeClass("d-none");
    $('#mdns_status').removeClass("border-0");
    $("#mdns_status").prop("disabled", false);
  }
  else {
    $('#mdns_buttons').addClass("d-none");
    $('#mdns_status').addClass("border-0");
    $("#mdns_status").prop("disabled", true);
  }
});

$('#mdns_refresh').on('click', function () {
  show_spinner();
  esp_get_mdns()
    .then(function () {
      hide_spinner(500);
    });
});

$('#mdns_save').on('click', function () {
  show_spinner();
  return esp_query({
    type: 'POST',
    url: '/api/mdns',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ mdns_enabled: Number($('#mdns_status').val()) }),
    success: function () {
      alert("mDNS status saved.");
      esp_get_mdns()
        .then(function () {
          hide_spinner(500);
        });
    },
    error: query_err
  });
});

// time&date

function esp_get_datetime() {
  return esp_query({
    type: 'GET',
    url: '/api/timedate',
    dataType: 'json',
    success: null,
    error: query_err
  });
}

function update_datetime(data) {
  return new Promise(function (resolve, reject) {
    $('#datetime_date').val(data.date);
    $('#datetime_sntp').val(data.sntp_enabled);
    $('#datetime_timezone').val(function () {
      if (data.timezone > 0) {
        return "+" + data.timezone;
      }
      else {
        return data.timezone;
      }
    });
    if ($('#datetime_buttons').hasClass("d-none")) {
      $("#datetime_sntp").prop("disabled", true);
      $("#datetime_timezone").prop("disabled", true);
    }
    if ($('#datetime_sntp').val() == 1) {
      $('#datetime_set').prop("disabled", true);
    } else {
      $('#datetime_set').prop("disabled", false);
    }
    resolve(data);
  })
}

function update_timevalue(data) {
  return new Promise(function (resolve, reject) {
    $('#datetime_date').val(data.date);
    resolve(data);
  })
}

// to stop asking datetime set this to 0
var device_running = 1;

function periodically_update_datetime() {
  if ($('#datetime_date').length == 0)
    device_running = 0;
  if (device_running) {
    // just update everything the time value,
    // periodically updating everything will make it
    // impossible to change options
    esp_get_datetime()
      .then(function (data) {
        update_timevalue(data);
      });
    setTimeout(function () {
      periodically_update_datetime();
    }, 10000);
  }
}

$('#datetime_edit').on('click', function () {
  if ($('#datetime_buttons').hasClass("d-none")) {
    $('#datetime_buttons').removeClass("d-none");
    $('#datetime_sntp').removeClass("border-0");
    $("#datetime_sntp").prop("disabled", false);
    $('#datetime_timezone').removeClass("border-0");
    $("#datetime_timezone").prop("disabled", false);
  }
  else {
    $('#datetime_buttons').addClass("d-none");
    $('#datetime_sntp').addClass("border-0");
    $("#datetime_sntp").prop("disabled", true);
    $('#datetime_timezone').addClass("border-0");
    $("#datetime_timezone").prop("disabled", true);
  }
});

$('#datetime_refresh').on('click', function () {
  show_spinner().then(function () {
    esp_get_datetime()
      .then(function (data) {
        update_datetime(data)
          .then(function () {
            hide_spinner(500)
          })
      })
  })
});

$('#datetime_save').on('click', function () {
  show_spinner();
  return esp_query({
    type: 'POST',
    url: '/api/timedate/cfg',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ sntp_enabled: Number($('#datetime_sntp').val()), timezone: Number($('#datetime_timezone').val()) }),
    success: function () {
      alert("Time & Date settings saved.");
      esp_get_datetime()
        .then(function (data) {
          update_datetime(data)
            .then(function () {
              hide_spinner(500)
            })
        })
    },
    error: query_err
  });
});

$('#datetime_set').on('click', function () {
  var now = new Date;
  // getTime() is UTC
  var now_utc = Math.floor(now.getTime() / 1000);
  show_spinner();
  return esp_query({
    type: 'POST',
    url: '/api/timedate',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ timestamp: now_utc }),
    success: function () {
      alert("Device Time & Date set to " + now.toUTCString());
      esp_get_datetime()
        .then(function (data) {
          update_datetime(data)
            .then(function () {
              hide_spinner(500)
            })
        })
    },
    error: query_err
  });
});

// OTA

function esp_get_ota() {
  return esp_query({
    type: 'GET',
    url: '/api/ota/cfg',
    dataType: 'json',
    success: update_ota,
    error: query_err
  });
}

function update_ota(data) {
  $("#ota_host").val(data.host);
  $("#ota_port").val(data.port);
  $("#ota_path").val(data.path);
  $("#ota_check_version").val(data.check_version);
  if ($('#ota_buttons').hasClass("d-none")) {
    $("#ota_host").prop("disabled", true);
    $("#ota_port").prop("disabled", true);
    $("#ota_path").prop("disabled", true);
    $("#ota_check_version").prop("disabled", true);
  }
  if (($('#ota_host').val() === "") || ($('#ota_port').val() == 0) || ($('#ota_path').val() === "")) {
    $('#ota_start').prop("disabled", true);
  } else {
    $('#ota_start').prop("disabled", false);
  }
}

$('#ota_edit').on('click', function () {
  if ($('#ota_buttons').hasClass("d-none")) {
    $('#ota_buttons').removeClass("d-none");
    $('#ota_host').removeClass("border-0");
    $("#ota_host").prop("disabled", false);
    $('#ota_port').removeClass("border-0");
    $("#ota_port").prop("disabled", false);
    $('#ota_path').removeClass("border-0");
    $("#ota_path").prop("disabled", false);
    $('#ota_check_version').removeClass("border-0");
    $("#ota_check_version").prop("disabled", false);
  }
  else {
    $('#ota_buttons').addClass("d-none");
    $('#ota_host').addClass("border-0");
    $("#ota_host").prop("disabled", true);
    $('#ota_port').addClass("border-0");
    $("#ota_port").prop("disabled", true);
    $('#ota_path').addClass("border-0");
    $("#ota_path").prop("disabled", true);
    $('#ota_check_version').addClass("border-0");
    $("#ota_check_version").prop("disabled", true);
  }
});

$('#ota_host').change(function () {
  $('#ota_start').prop("disabled", true);
});

$('#ota_port').change(function () {
  $('#ota_start').prop("disabled", true);
});

$('#ota_path').change(function () {
  $('#ota_start').prop("disabled", true);
});

$('#ota_check_version').change(function () {
  $('#ota_start').prop("disabled", true);
});

$('#ota_refresh').on('click', function () {
  show_spinner().then(function () {
    esp_get_ota()
      .then(function () {
        hide_spinner(500)
      });
  });
});

$('#ota_save').on('click', function () {
  return esp_query({
    type: 'POST',
    url: '/api/ota/cfg',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ host: $('#ota_host').val(), port: Number($('#ota_port').val()), path: $('#ota_path').val(), check_version: Number($('#ota_check_version').val()), reboot_on_completion: 1 }),
    success: function () {
      alert("OTA settings saved.");
      esp_get_ota()
        .then(function () {
          hide_spinner(500)
        });
    },
    error: query_err
  });
});

$('#ota_start').on('click', function () {
  device_running = 0;
  show_spinner()
    .then(function () {
      return esp_query({
        type: 'POST',
        url: '/api/ota',
        dataType: 'json',
        timeout: 18000,
        success: function (data) {
          if ((data.msg).includes("ebooting")) {
            device_running = 0;
          } else {
            device_running = 1;
          }
          alert("" + data.msg);
          hide_spinner(500);
        },
        error: query_err
      });
    });
});

// diagnostic

function esp_get_diag() {
  return esp_query({
    type: 'GET',
    url: '/api/diagnostic/cfg',
    dataType: 'json',
    success: update_diag,
    error: query_err
  });
}

function update_diag(data) {
  $('#diag_led').val(data.diag_led_mask);
  $('#diag_serial').val(data.serial_log_mask);
  $('#diag_bitrate').val(data.uart_0_bitrate);
  $('#diag_sdk_print').val(data.sdk_print_enabled);
  if ($('#diag_buttons').hasClass("d-none")) {
    $("#diag_led").prop("disabled", true);
    $("#diag_serial").prop("disabled", true);
    $("#diag_bitrate").prop("disabled", true);
    $("#diag_sdk_print").prop("disabled", true);
  }
}

$('#diag_edit').on('click', function () {
  if ($('#diag_buttons').hasClass("d-none")) {
    $('#diag_buttons').removeClass("d-none");
    $('#diag_led').removeClass("border-0");
    $("#diag_led").prop("disabled", false);
    $('#diag_serial').removeClass("border-0");
    $("#diag_serial").prop("disabled", false);
    $('#diag_bitrate').removeClass("border-0");
    $("#diag_bitrate").prop("disabled", false);
    $('#diag_sdk_print').removeClass("border-0");
    $("#diag_sdk_print").prop("disabled", false);
  }
  else {
    $('#diag_buttons').addClass("d-none");
    $('#diag_led').addClass("border-0");
    $("#diag_led").prop("disabled", true);
    $('#diag_serial').addClass("border-0");
    $("#diag_serial").prop("disabled", true);
    $('#diag_bitrate').addClass("border-0");
    $("#diag_bitrate").prop("disabled", true);
    $('#diag_sdk_print').addClass("border-0");
    $("#diag_sdk_print").prop("disabled", true);
  }
});

$('#diag_refresh').on('click', function () {
  show_spinner().then(function () {
    esp_get_diag()
      .then(function () {
        hide_spinner(500)
      });
  });
});

$('#diag_save').on('click', function () {
  show_spinner().then(function () {
    return esp_query({
      type: 'POST',
      url: '/api/diagnostic/cfg',
      dataType: 'json',
      contentType: 'application/json',
      data: JSON.stringify({ diag_led_mask: Number($('#diag_led').val()), serial_log_mask: Number($('#diag_serial').val()), uart_0_bitrate: Number($('#diag_bitrate').val()), sdk_print_enabled: Number($('#diag_sdk_print').val()) }),
      success: function () {
        alert("Diagnostic settings saved.");
        esp_get_diag()
          .then(function () {
            hide_spinner(500)
          });
      },
      error: query_err
    });
  });
});