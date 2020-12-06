// gpio.js

$(document).ready(function () {
  update_gpio_list();
});

function update_gpio_list() {
  show_spinner().then(function (data) {
    update_gpio(1).then(function () {
      update_gpio(2).then(function () {
        update_gpio(3).then(function () {
          update_gpio(4).then(function () {
            update_gpio(5).then(function () {
              update_gpio(6).then(function () {
                update_gpio(7).then(function () {
                  update_gpio(8).then(function () {
                    hide_spinner(500);
                  });
                });
              });
            });
          });
        });
      });
    });
  });
}

function esp_get_gpio_cfg(ii) {
  return esp_query({
    type: 'GET',
    url: '/api/gpio/cfg/' + ii,
    dataType: 'json',
    success: null,
    error: query_err
  });
}

function esp_get_gpio_level(ii) {
  return esp_query({
    type: 'GET',
    url: '/api/gpio/' + ii,
    dataType: 'json',
    success: null,
    error: query_err
  });
}

function update_gpio_level(idx, data) {
  var gpio_level_id = "#d" + idx + "_value";
  if (data.gpio_level == "low")
    $(gpio_level_id).prop('checked', true);
  else
    $(gpio_level_id).prop('checked', false);
}

function update_gpio(idx) {
  return new Promise(function (resolve, reject) {
    var gpio_cfg_id = "#d" + idx + "_cfg";
    var gpio_level_id = "#d" + idx + "_value";
    esp_get_gpio_cfg(idx)
      .then(function (data) {
        switch (data.gpio_type) {
          case "unprovisioned":
            $(gpio_cfg_id).val(0);
            $(gpio_level_id).prop('disabled', true);
            resolve("gpio " + idx + " updated");
            break;
          case "input":
            $(gpio_cfg_id).val(1);
            $(gpio_level_id).prop('disabled', true);
            esp_get_gpio_level(idx)
              .then(function (data) {
                update_gpio_level(idx, data);
                resolve("gpio " + idx + " updated");
              })
              .catch(function () {
                reject("gpio not updated");
              });
            break;
          case "output":
            $(gpio_cfg_id).val(2);
            $(gpio_level_id).prop('disabled', false);
            esp_get_gpio_level(idx)
              .then(function (data) {
                update_gpio_level(idx, data);
                resolve("gpio " + idx + " updated");
              })
              .catch(function () {
                reject("gpio not updated");
              });
            break;
          default:
            reject("gpio not updated");
        }
      });
  });
}

$('#gpio_refresh').on('click', function () {
  update_gpio_list();
});

function esp_set_gpio_level(ii, value) {
  show_spinner()
    .then(function () {
      return esp_query({
        type: 'POST',
        url: '/api/gpio/' + ii,
        dataType: 'json',
        contentType: 'application/json',
        data: JSON.stringify({ gpio_level: value }),
        success: function (data) {
          update_gpio(ii)
            .then(function () {
              hide_spinner(500);
            });
        },
        error: query_err
      });
    });
}

function gpio_set(idx) {
  var gpio_level_id = "#d" + idx + "_value";
  if ($(gpio_level_id).prop('checked'))
    esp_set_gpio_level(idx, "low");
  else
    esp_set_gpio_level(idx, "high");
}

function esp_set_gpio_cfg(ii, value) {
  show_spinner()
    .then(function () {
      return esp_query({
        type: 'POST',
        url: '/api/gpio/cfg/' + ii,
        dataType: 'json',
        contentType: 'application/json',
        data: JSON.stringify({ gpio_type: value }),
        success: function () {
          update_gpio(ii)
            .then(function () {
              hide_spinner(500);
            });
        },
        error: query_err
      });
    });
}

function gpio_cfg(idx) {
  var gpio_cfg_id = "#d" + idx + "_cfg";
  var value = $(gpio_cfg_id).val();
  switch (value) {
    case "0":
      esp_set_gpio_cfg(idx, "unprovisioned");
      var gpio_level_id = "#d" + idx + "_value";
      $(gpio_level_id).prop('checked', false);
      break;
    case "1":
      esp_set_gpio_cfg(idx, "input");
      break;
    case "2":
      esp_set_gpio_cfg(idx, "output");
      break;
  }
}