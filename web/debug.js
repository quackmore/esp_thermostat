// debug.js

// spinner while awaiting for page load
$(document).ready(function () {
  setTimeout(function () {
    $('#awaiting').modal('hide');
  }, 1000);
  update_page();
});

function update_page() {
  update_last_rst();
  update_meminfo();
  file_upload_update();
  setTimeout(function () {
    update_fsinfo();
    update_file_list();
  }, 250);
}

// reboot

$('#reboot').on('click', function () {
  if (confirm("Confirm device reboot...")) {
    $.ajax({
      type: 'POST',
      url: esp8266.url + '/api/reboot',
      dataType: 'json',
      crossDomain: esp8266.cors,
      timeout: 2000,
      success: function (data) {
        alert("" + data.msg);
      },
      error: function (jqXHR, textStatus, errorThrown) {
        ajax_error(jqXHR, textStatus, errorThrown);
      }
    })
  }
});

// last reboot

function esp_get_last_rst(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/debug/lastReset',
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

function formatStackDump(data) {
  var stackDump = "";
  for (var idx = 0; idx < data.spDump.length; idx++) {
    stackDump += data.spDump[idx];
    stackDump += '\r\n';
  }
  return stackDump;
}

function update_last_rst() {
  esp_get_last_rst(function (data) {
    $("#last_rst_date").val(data.date);
    $("#last_rst_reason").val(data.reason);
    $("#last_rst_exccause").val(data.exccause);
    $("#last_rst_epc1").val(data.epc1);
    $("#last_rst_epc2").val(data.epc2);
    $("#last_rst_epc3").val(data.epc3);
    $("#last_rst_evcvaddr").val(data.evcvaddr);
    $("#last_rst_depc").val(data.depc);
    $("#last_rst_sp").val(data.sp);
    $("#last_rst_spdump").val(formatStackDump(data));
  });
}

$('#last_rst_refresh').on('click', function (e) {
  update_last_rst();
});

// meminfo

function esp_get_meminfo(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/debug/memInfo',
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

function update_meminfo() {
  esp_get_meminfo(function (data) {
    $("#meminfo_stack_max").val(data.stack_max_addr);
    $("#meminfo_stack_min").val(data.stack_min_addr);
    $("#meminfo_heap_start").val(data.heap_start_addr);
    $("#meminfo_heap_free").val(data.heap_free_size);
    $("#meminfo_heap_max_size").val(data.heap_max_size);
    $("#meminfo_heap_min_size").val(data.heap_min_size);
    $("#meminfo_heap_objs").val(data.heap_objs);
    $("#meminfo_heap_max_objs").val(data.heap_max_objs);
  });
}

$('#meminfo_refresh').on('click', function (e) {
  update_meminfo();
});

// SPIFFS

function esp_get_fsinfo(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/fs',
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

function update_fsinfo() {
  esp_get_fsinfo(function (data) {
    $("#fsinfo_size").val(data.file_system_size);
    $("#fsinfo_used").val(data.file_system_used_size);
  });
}

$('#fsinfo_refresh').on('click', function () {
  update_fsinfo();
});

$('#fs_format').on('click', function () {
  if (confirm("File system content will be lost.\nConfirm format...")) {
    $.ajax({
      type: 'POST',
      url: esp8266.url + '/api/fs/format',
      dataType: 'json',
      crossDomain: esp8266.cors,
      timeout: 2000,
      success: function (data) {
        alert("" + data.msg);
      },
      error: function (jqXHR, textStatus, errorThrown) {
        ajax_error(jqXHR, textStatus, errorThrown);
      }
    })
  }
});

// Files

function esp_get_file(success_cb) {
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/file',
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

function update_file_list() {
  esp_get_file(function (data) {
    data.files.sort(function (a, b) {
      var nameA = a.name.toUpperCase();
      var nameB = b.name.toUpperCase();
      if (nameA < nameB) {
        return -1;
      }
      if (nameA > nameB) {
        return 1;
      }
      return 0;
    });
    var file_list_str = "";
    for (var idx = 0; idx < data.files.length; idx++) {
      file_list_str += '<div class="row no-gutters"><span class="my-auto float-left">';
      file_list_str += data.files[idx].name;
      file_list_str += '</span><div class="btn-group my-auto ml-auto" role="group"><button class="btn" disabled>';
      file_list_str += data.files[idx].size;
      file_list_str += '</button><button class="btn" onclick="file_show(\x27';
      file_list_str += data.files[idx].name;
      file_list_str += '\x27,';
      file_list_str += data.files[idx].size;
      file_list_str += ');"><i class="fa fa-file-o"></i></button><button class="btn" onclick="file_del(\x27';
      file_list_str += data.files[idx].name;
      file_list_str += '\x27);"><i class="fa fa-trash"></i></button></div></div>';
    }
    $("#file_list").html(file_list_str);
  });
}

$('#file_refresh').on('click', function () {
  update_file_list();
});

function file_show(file, filesize) {
  $('#awaiting').modal('show');
  var timeout_value = filesize / 8 + 3000;
  console.log(timeout_value);
  $.ajax({
    type: 'GET',
    url: esp8266.url + '/api/file/' + file,
    crossDomain: esp8266.cors,
    timeout: timeout_value,
    success: function (data) {
      setTimeout(function () {
        $('#awaiting').modal('hide');
      }, 1000);
      $('#file_name').text(file);
      $('#file_content').val(data);
      $('#file_modal').modal('show');
    },
    error: function (jqXHR, textStatus, errorThrown) {
      setTimeout(function () {
        $('#awaiting').modal('hide');
      }, 1000);
      ajax_error(jqXHR, textStatus, errorThrown);
    }
  })
}

function file_del(file) {
  if (confirm("Deleted files cannot be recovered.\nConfirm delete...")) {
    $.ajax({
      type: 'DELETE',
      url: esp8266.url + '/api/file/' + file,
      crossDomain: esp8266.cors,
      timeout: 2000,
      success: function (data) {
        // alert("" + data.msg);
        update_file_list();
      },
      error: function (jqXHR, textStatus, errorThrown) {
        ajax_error(jqXHR, textStatus, errorThrown);
        update_file_list();
      }
    })
  }
}

// File upload
function file_upload_update() {
  $("#file_upload").prop("disabled", true);
}

$("#file_choose").on("change", function () {
  var fileName = $(this).val().split("\\").pop();
  $(this).siblings(".custom-file-label").addClass("selected").html(fileName);
  if (fileName) {
    $("#file_upload").prop("disabled", false);
  } else {
    $("#file_upload").prop("disabled", true);
  }
});

$("#file_upload").on("click", function () {
  var file = $("#file_choose").prop("files")[0];
  var fr = new FileReader();
  fr.readAsArrayBuffer(file);
  fr.onload = function () {
    $('#awaiting').modal('show');
    var buffer = new Uint8Array(fr.result);
    var end;
    if (buffer.length < 1500) {
      end = buffer.length;
    }
    else {
      end = 1500;
    }
    $.ajax({
      type: 'POST',
      url: esp8266.url + '/api/file/' + $("#file_choose").val().split("\\").pop(),
      contentType: 'application/octet-stream',
      data: buffer.slice(0, end),
      processData: false,
      crossDomain: esp8266.cors,
      timeout: 2000,
      success: function (data) {
        if (buffer.length < 1500) {
          setTimeout(function () {
            $('#awaiting').modal('hide');
          }, 1000);
          update_file_list();
        }
        else {
          upload_remaining(buffer, 1500);
        }
      },
      error: function (jqXHR, textStatus, errorThrown) {
        ajax_error(jqXHR, textStatus, errorThrown);
        setTimeout(function () {
          $('#awaiting').modal('hide');
        }, 1000);
        update_file_list();
      }
    });
  }
});

function upload_remaining(buffer, starting_point) {
  var end;
  if ((buffer.length - starting_point) < 1500) {
    end = buffer.length;
  }
  else {
    end = starting_point + 1500;
  }
  $.ajax({
    type: 'PUT',
    url: esp8266.url + '/api/file/' + $("#file_choose").val().split("\\").pop(),
    contentType: 'application/octet-stream',
    data: buffer.slice(starting_point, end),
    processData: false,
    crossDomain: esp8266.cors,
    timeout: 2000,
    success: function (data) {
      if ((buffer.length - starting_point) < 1500) {
        setTimeout(function () {
          $('#awaiting').modal('hide');
        }, 1000);
        update_file_list();
      }
      else {
        // setTimeout(upload_remaining(buffer, (starting_point + 1500)), 2000);
        upload_remaining(buffer, (starting_point + 1500));
      }
    },
    error: function (jqXHR, textStatus, errorThrown) {
      ajax_error(jqXHR, textStatus, errorThrown);
      setTimeout(function () {
        $('#awaiting').modal('hide');
      }, 1000);
      update_file_list();
    }
  });
}

// memhexdump

$('#memhexdump_start_addr').on('change', function () {
  $('#memhexdump_start_addr').val(($('#memhexdump_start_addr').val()).toUpperCase());
});

$('#memhexdump_length').on('change', function () {
  if ($('#memhexdump_length').val() < 4) {
    $('#memhexdump_length').val(4);
  }
  if ($('#memhexdump_length').val() > 1024) {
    $('#memhexdump_length').val(1024);
  }
});

function esp_get_memhexdump(success_cb) {
  $.ajax({
    type: 'POST',
    url: esp8266.url + '/api/debug/hexMemDump',
    dataType: 'json',
    contentType: 'application/json',
    data: JSON.stringify({ address: $('#memhexdump_start_addr').val(), length: Number($('#memhexdump_length').val()) }),
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

function format_hex(unformatted_str) {
  // calculate how many 32 bits words will fit on a line
  $('#sample_text').removeClass('d-none');
  var textarea_width = (document.getElementById("memhexdump_content")).offsetWidth;
  var text_width = (document.getElementById("sample_text")).offsetWidth;
  var word32_on_a_line = Math.floor((textarea_width - 30 - text_width) / ((text_width * 12) / 10));
  $('#sample_text').addClass('d-none');
  unformatted_str = unformatted_str + " ";
  // start each line with the address
  var formatted_str = (($('#memhexdump_start_addr').val()).substr(-8)).toUpperCase() + "| ";
  var start = 1;
  var end = unformatted_str.indexOf(" ", start);
  var byte_count = 0;
  while (end >= 0) {
    // format current byte
    formatted_str += ("00" + unformatted_str.substr(start, (end - start))).substr(-2);
    // find next byte
    start = end + 1;
    end = unformatted_str.indexOf(" ", start);
    // check if a line has been completed and add \r\n and the new address
    byte_count += 1;
    if ((byte_count % (word32_on_a_line * 4)) == 0) {
      formatted_str += "\r\n";
      formatted_str += ((parseInt($('#memhexdump_start_addr').val(), 16) + byte_count).toString(16)).toUpperCase() + "| ";
      word16_len = 0;
    }
    else {
      formatted_str += " ";
    }
  }
  return formatted_str;
}

function format_chars(unformatted_str) {
  // calculate how many 32 bits words will fit on a line
  $('#sample_text').removeClass('d-none');
  var textarea_width = (document.getElementById("memhexdump_content")).offsetWidth;
  var text_width = (document.getElementById("sample_text")).offsetWidth;
  var word32_on_a_line = Math.floor((textarea_width - 30 - text_width) / ((text_width * 8) / 10));
  $('#sample_text').addClass('d-none');
  unformatted_str = unformatted_str + " ";
  // start each line with the address
  var formatted_str = (($('#memhexdump_start_addr').val()).substr(-8)).toUpperCase() + "| ";
  var start = 1;
  var end = unformatted_str.indexOf(" ", start);
  var byte_count = 0;
  var curr_char;
  while (end >= 0) {
    curr_char = unformatted_str.substr(start, (end - start));
    if ((parseInt(curr_char, 16) <= 0x1F) || (parseInt(curr_char, 16) >= 0x7F)) {
      curr_char = "2E";
    }
    formatted_str += String.fromCharCode(parseInt(curr_char, 16));
    start = end + 1;
    end = unformatted_str.indexOf(" ", start);
    byte_count += 1;
    if ((byte_count % (word32_on_a_line * 8)) == 0) {
      formatted_str += "\r\n";
      formatted_str += ((parseInt($('#memhexdump_start_addr').val(), 16) + byte_count).toString(16)).toUpperCase() + "| ";
    }
  }
  return formatted_str;
}

function update_memhexdump() {
  esp_get_memhexdump(function (data) {
    if ($('#memhexdump_format').val() == 1)
      $("#memhexdump_content").val(format_chars(data.content));
    else
      $("#memhexdump_content").val(format_hex(data.content));
  });
}

$('#memhexdump_refresh').on('click', function () {
  update_memhexdump();
});