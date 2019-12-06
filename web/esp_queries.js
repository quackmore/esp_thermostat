const esp8266 = {
  // "url": "http://192.168.1.185",
  // "cors": true
  "url": "",
  "cors": false
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

export { get_current_vars, get_settings, save_settings, get_rl_settings, save_rl_settings, get_adv_ctrl_settings, save_adv_ctrl_settings };