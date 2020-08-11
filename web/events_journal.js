// events_journal.js

// spinner while awaiting for page load
$(document).ready(function () {
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_page();
});

function update_page() {
  update_diagnostic_events();
}

// Events Journal

function esp_get_diagnostic_events(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/diagnostic',
    dataType: 'json',
    crossDomain: esp8266.cors,
    timeout: 5000,
    success: function (data) {
      success_cb(data);
      setTimeout(function () {
        $('.modal').modal('hide');
      }, 1000);
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
      setTimeout(function () {
        $('.modal').modal('hide');
      }, 1000);
    }
  });
}

function update_diagnostic_events() {
  esp_get_diagnostic_events(update_diagnostic_events_table);
}

function update_diagnostic_events_table(data) {
  $("#events_table").empty();
  $("#events_table").append('<thead><tr><th scope="col">Timestamp</th><th scope="col">Type</th><th scope="col">Code</th><th scope="col">Desc</th><th scope="col">Value</th></tr></thead><tbody>');
  for (var ii = 0; ii < data.diag_events.length; ii++) {
    var ts = new Date(data.diag_events[ii].ts * 1000);
    if (data.diag_events[ii].ack == 1) {
      $("#events_table").append('<tr class="text-success"><td>' + ts.toString().substring(4, 24) + '</td><td>' + get_evnt_str(data.diag_events[ii].type) + '</td><td>' + String("0000" + data.diag_events[ii].code).slice(-4) + '</td><td>' + get_code_str(data.diag_events[ii].code) + '</td><td>' + data.diag_events[ii].val + '</td></tr>');
    }
    else {
      $("#events_table").append('<tr><td>' + ts.toString().substring(4, 24) + '</td><td>' + get_evnt_str(data.diag_events[ii].type) + '</td><td>' + String("0000" + data.diag_events[ii].code).slice(-4) + '</td><td>' + get_code_str(data.diag_events[ii].code) + '</td><td>' + data.diag_events[ii].val + '</td></tr>');
    }
  }
  $("#events_table").append('</tbody>');
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

$('#events_refresh').on('click', function () {
  $('#awaiting').modal('show');
  update_diagnostic_events();
});

$('#events_ack').on('click', function () {
  esp_ack_diagnostic_events(function (xhr) {
    alert("" + JSON.parse(xhr.responseText).error.reason);
  });
  update_diagnostic_events();
});

function esp_ack_diagnostic_events(error_cb) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/diagnostic',
    crossDomain: esp8266.cors,
    timeout: 2000,
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  });
}