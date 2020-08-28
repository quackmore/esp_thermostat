// device.js

// spinner while awaiting for page load
$(document).ready(function () {
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_page();
});

function update_page() {
  update_device_info();
  update_wifi();
  update_cron();
  setTimeout(function () {
    update_mdns();
    update_datetime();
    update_diag();
  }, 250);
  setTimeout(function () {
    periodically_update_datetime();
    update_ota();
  }, 500);
}

// device info

function esp_get_info(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/info',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_device_info() {
  esp_get_info(function (data) {
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
  });
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
  update_device_info();
});

$('#info_save').on('click', function (e) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/deviceName',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ device_name: $('#dev_name').val() }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function () {
      alert("Device name saved.");
      update_device_info();
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
});

// wifi

function esp_wifi_info(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/wifi',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_wifi() {
  esp_wifi_info(function (data) {
    $("#wifi_working_as").val(data.op_mode);
    $("#wifi_ssid_label").text(function () {
      if (data.op_mode === "STATION")
        return "Connected to";
      else
        return "SSID";
    });
    $("#wifi_ssid").val(data.SSID);
    $("#wifi_ip").val(data.ip_address);
  });
}

$('#wifi_edit').on('click', function () {
  if ($('#wifi_buttons').hasClass("d-none")) {
    $('#wifi_buttons').removeClass("d-none");
    update_wifi_aps();
  }
  else {
    $('#wifi_buttons').addClass("d-none");
  }
});

function esp_wifi_scan(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/wifi/scan',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 10000,
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_wifi_aps() {
  $("#wifi_aps").empty();
  $("#wifi_aps").append('<option selected><span class="sr-only">Loading...</span></option>');
  esp_wifi_scan(function (data) {
    $("#wifi_aps").empty();
    for (var ii = 0; ii < data.AP_SSIDs.length; ii++) {
      if ($("#wifi_ssid").val() === data.AP_SSIDs[ii]) {
        $("#wifi_aps").append("<option value='" + ii + "' selected>" + data.AP_SSIDs[ii] + "</option>");
      }
      else {
        $("#wifi_aps").append("<option value='" + ii + "'>" + data.AP_SSIDs[ii] + "</option>");
      }
    }
  });
}

$('#wifi_refresh').on('click', function (e) {
  update_wifi_aps();
});

$('#wifi_connect').on('click', function (e) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/wifi/cfg',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ station_ssid: $('#wifi_aps option:selected').text(), station_pwd: $('#wifi_pwd').val() }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (xhr) {
      alert("Conneting to AP " + $('#wifi_aps option:selected').text() + " ...");
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
});

// cron

function esp_get_cron(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/cron',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_cron() {
  esp_get_cron(function (data) {
    $('#cron_status').val(data.cron_enabled);
    if ($('#cron_buttons').hasClass("d-none")) {
      $("#cron_status").prop("disabled", true);
    }
  });
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
  update_cron();
});

$('#cron_save').on('click', function () {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/cron',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ cron_enabled: Number($('#cron_status').val()) }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function () {
      alert("Cron status saved.");
      update_cron();
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
});

// mDNS

function esp_get_mdns(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/mdns',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_mdns() {
  esp_get_mdns(function (data) {
    $('#mdns_status').val(data.mdns_enabled);
    if ($('#mdns_buttons').hasClass("d-none")) {
      $("#mdns_status").prop("disabled", true);
    }
  });
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
  update_mdns();
});

$('#mdns_save').on('click', function () {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/mdns',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ mdns_enabled: Number($('#mdns_status').val()) }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function () {
      alert("mDNS status saved.");
      update_mdns();
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
});

// time&date

function esp_get_datetime(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/timedate',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_datetime() {
  esp_get_datetime(function (data) {
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
  });
}

// to stop asking datetime set this to 0
var device_running = 1;

function periodically_update_datetime() {
  if (device_running) {
    esp_get_datetime(function (data) {
      $('#datetime_date').val(data.date);
    });
  }
  setTimeout(function () {
    periodically_update_datetime();
  }, 10000);
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
  update_datetime();
});

$('#datetime_save').on('click', function () {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/timedate/cfg',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ sntp_enabled: Number($('#datetime_sntp').val()), timezone: Number($('#datetime_timezone').val()) }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function () {
      alert("Time & Date settings saved.");
      update_datetime();
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
});

$('#datetime_set').on('click', function () {
  var now = new Date;
  // getTime() is UTC
  var now_utc = Math.floor(now.getTime() / 1000);
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/timedate',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ timestamp: now_utc }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function () {
      alert("Device Time & Date set to " + now.toUTCString());
      update_datetime();
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
});

// OTA

function esp_get_ota(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/ota/cfg',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_ota() {
  esp_get_ota(function (data) {
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
  });
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
  update_ota();
});

$('#ota_save').on('click', function () {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/ota/cfg',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ host: $('#ota_host').val(), port: Number($('#ota_port').val()), path: $('#ota_path').val(), check_version: Number($('#ota_check_version').val()), reboot_on_completion: 1 }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function () {
      alert("OTA settings saved.");
      update_ota();
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
});

$('#ota_start').on('click', function () {
  $('#awaiting').modal('show');
  device_running = 0;
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/ota',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 18000,
    success: function (data) {
      setTimeout(function () {
        $('#awaiting').modal('hide');
      }, 1000);
      alert("" + data.msg);
      if ((data.msg).includes("ebooting")) {
        device_running = 0;
      } else {
        device_running = 1;
      }
    },
    error: function (jqXHR, textStatus, errorThrown) {
      setTimeout(function () {
        $('#awaiting').modal('hide');
      }, 1000);
      ajax_error(jqXHR, textStatus, errorThrown);
      device_running = 1;
    }
  });
});

// diagnostic

function esp_get_diag(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/diagnostic/cfg',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      success_cb(data);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}

function update_diag() {
  esp_get_diag(function (data) {
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
  });
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
  update_diag();
});

$('#diag_save').on('click', function () {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/diagnostic/cfg',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ diag_led_mask: Number($('#diag_led').val()), serial_log_mask: Number($('#diag_serial').val()), uart_0_bitrate: Number($('#diag_bitrate').val()), sdk_print_enabled: Number($('#diag_sdk_print').val()) }),
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function () {
      alert("Diagnostic settings saved.");
      update_diag();
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
});