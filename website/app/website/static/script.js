function initSite() {
  site = {
    $messageBox: $('#message-box'),
    $alertBox: $('#connection-error')
  };

  pingDevice();
  setInterval(pingDevice, 60000);

  /* "switch on" buttons */
  $('input[id^="btn-on-"]').click(function()  {
    switchSocket('on', $(this).data('socket-id'));
  });

  /* "switch off" buttons */
  $('input[id^="btn-off-"]').click(function() {
    switchSocket('off', $(this).data('socket-id'));
  });

  /* description texts */
  $('.description-text').click(function(event) {
    showDescriptionInput(event);
  });

  /* "save" description buttons */
  $('input[value=Save]').click(function(event) {
    saveDescription(event);
  });

  /* "cancel" description buttons */
  $('input[value=Cancel]').click(cancelDescriptionEdit);

  /* enter/escape in description inputs */
  $('.description-input').find('input[type=text]').keyup(function(event)  {
    if (event.keyCode == 13)  {
      $(event.target).parent().find('input[value=Save]').click();
    }
    else if (event.keyCode == 27)  {
      $(event.target).parent().find('input[value=Cancel]').click();
    }
    return false;
  });

  /* create-new-task link */
  $('#new-task-link').click(showTaskForm);
  /* save-task link */
  $('a[name=save-task-link]').click(saveTask);
  /* delete-task link */
  $('a[name=delete-task-link]').click(deleteTask);

  convertTaskTimes();
  convertLogTimes();
}

function pingDevice() {
  $.ajax({
    url: endpoints['ping-device'],
    dataType: 'json',
    success: function(json) {
      if (json.status == 'ok')  {
        site.$messageBox.fadeIn();
        site.$alertBox.hide();
      } else {
        var error = '(Address: ' + remoteAddr + ', Error: ';
          error += json.response + ')';
          site.$alertBox.find('span').text(error);
          site.$alertBox.fadeIn();
          site.$messageBox.hide();
      }
      console.log(json);
    },
    error: function(xhr, status, errorThrown) {
      console.log('Error: ' + errorThrown);
      console.log('Status: ' + status);
      site.$alertBox.show();
      site.$messageBox.hide();
    }
  });
}

function switchSocket(action, socket) {
  $.ajax({
    url: endpoints['switch-socket'],
    type: 'POST',
    dataType: 'json',
    contentType: 'application/json; charset=utf-8',
    data: JSON.stringify({
      socket: socket,
      action: action
    }),
    success: function(json) {
      console.log(json);

      if (json.status == 'ok')  {
        site.$alertBox.hide();
        $('#success-img-' + socket).fadeIn('fast').delay(1000).fadeOut('slow');

      } else {
        var error = '(Address: ' + remoteAddr + ', Error: ';
        error += json.response + ')';
        site.$alertBox.find('span').text(error);
        site.$alertBox.show();
      }
    },
    error: function(xhr, status, errorThrown) {
      console.log('Error: ' + errorThrown);
      console.log('Status: ' + status);
    }
  });
}

function showDescriptionInput(event) {
  $this = $(event.target);
  $input = $this.parent().find('.description-input').find('input[type=text]');

  $input.val($this.text());

  $this.hide();
  $input.parent().show();
  $input.focus();
  $input.select();
}

function saveDescription(event)  {
  $this = $(event.target);
  socketId = $this.data('save-btn-for');
  $description = $this.parent().parent().find('.description-text');

  $.ajax({
    url: endpoints['change-description'],
    type: 'POST',
    dataType: 'json',
    contentType: 'application/json; charset=utf-8',
    data: JSON.stringify({
      socket: socketId,
      description: $('input[data-desc-input-for='+socketId+']').val()
    }),
    success: function(json) {
      console.log(json);
      if (json.status == 'ok')  {
        $description.text(json.description);
        $('#success-img-' + socketId).fadeIn('fast').delay(1000).fadeOut('slow');
      } else {
        $('#description-change-failed').hide();
        $('#description-change-failed').fadeIn();
      }
    },
    error: function(xhr, status, errorThrown) {
      console.log('Error: ' + errorThrown);
      console.log('Status: ' + status);
    },
    complete: function()  {
      $this.parent().hide();
      $description.show();
    }
  });
}

function cancelDescriptionEdit(event) {
  $button = $(event.target);
  $button.parent().hide();
  $button.parent().parent().find('.description-text').show();
}

function showTaskForm(event)  {
  event.preventDefault();
  var template = $('#form-template').text();
  $('#empty-schedule-row').hide();
  $('#user-schedule').find('tbody').append(template);

  $('a[name=save-task-link]').off('click');
  $('a[name=save-task-link]').click(saveTask);
  $('a[name=delete-task-link]').off('click');
  $('a[name=delete-task-link]').click(deleteTask);
}

function saveTask(event)  {
  event.preventDefault();
  var $link = $(event.target);
  var $row = $link.parent().parent();

  var $time = $row.find('input[name=time]');
  var $sockets = $row.find('select');
  var $active = $row.find('input[name=active]');
  var $taskId = $row.find('input[name=task-id]');
  var $idField = $row.find('td[name=task-id-field]');

  var turnOn = [], turnOff = [];

  /* iterate over selects */
  $sockets.each(function()  {
    if ($(this).val() == 0)
      turnOff.push(parseInt($(this).attr('name')));
    if ($(this).val() == 1)
      turnOn.push(parseInt($(this).attr('name')));
  });

  /* get time input */
  var hours = $time.val().split(':')[0];
  var minutes = $time.val().split(':')[1];

  /* check time input and indicate error to user */
  if (!(hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59)) {
    var color = $time.parent().css('background-color');
    $time.parent().animate({'background-color': '#a94442'}, 'slow', 'swing', function()  {
      $(this).animate({'background-color': color}, 'slow')});
    return;
  }

  /* convert to utc */
  var time = locale2utc(hours, minutes);

  /* check if at least one action is specified */
  // TODO

  var request = JSON.stringify({
    time: time.hours + ':' + time.minutes,
    active: $active.prop('checked'),
    turnOn: turnOn,
    turnOff: turnOff,
    taskId: parseInt($taskId.val())
  });

  $.ajax({
    url: endpoints['save-task'],
    type: 'POST',
    dataType: 'json',
    contentType: 'application/json; charset=utf-8',
    data: request,
    success: function(json) {
      if (json.status == 'ok')  {
        $taskId.val(json.taskId);
        $idField.text('#' + json.taskId);
        animateBackground($row, 'success');
      } else {
        animateBackground($row, 'error');
        console.log(json);
      }
    },
    error: function(xhr, status, errorThrown) {
      animateBackground($row, 'error');
      console.log(status, errorThrown);
    }
  });
}

function deleteTask(event)  {
  event.preventDefault();
  var $link = $(event.target);
  var $row = $link.parent().parent();
  var id = parseInt($row.find('input[name=task-id]').val());

  if (!window.confirm('Do you want to delete this Task?')) {
    return;
  }

  if (id == 0)  {
    /* task not in database */
    $row.remove();
    return;
  } else {
    $.ajax({
      url: endpoints['delete-task'],
      type: 'POST',
      dataType: 'json',
      contentType: 'application/json; charset=utf-8',
      data: JSON.stringify({taskId: id}),
      success: function(json) {
        if (json.status == 'ok')  {
          $row.remove();
        } else {
          animateBackground($row, 'error');
          console.log(json);
        }
      },
      error: function(xhr, status, errorThrown) {
        console.log(status, errorThrown);
      }
    });
  }
}


function animateBackground($el, status) {
  var bgColor = $el.css('background-color');
  if (status == 'success')  {
    $el.animate({'background-color': '#31708f'}, 'slow', 'swing', function()  {
      $el.animate({'background-color': bgColor}, 'slow')});
  } else {
    $el.animate({'background-color': '#a94442'}, 'slow', 'swing', function()  {
      $el.animate({'background-color': bgColor}, 'slow')});
  }
}

/* convert utc times back to locale time */
function convertTaskTimes() {
  $('input[name=time]').each(function() {
    var $input = $(this);
    var hours = $input.val().split(':')[0];
    var minutes = $input.val().split(':')[1];

    var localeTime = utc2locale(hours, minutes)

    hours = localeTime.hours<10 ? '0'+localeTime.hours : localeTime.hours;
    minutes = localeTime.minutes<10 ? '0'+localeTime.minutes : localeTime.minutes;
    $input.val(hours + ':' + minutes);
  });
}

/* convert timestamps to locale repr */
function convertLogTimes()  {
  $('td[name=task-log-time').each(function()  {
    var datetime = new Date(parseFloat($(this).text()) * 1000);
    $(this).text(datetime.toLocaleFormat());
  });
}

function locale2utc(hours, minutes) {
  var date = new Date();
  date.setHours(parseInt(hours), parseInt(minutes));
  return {hours:date.getUTCHours(), minutes:date.getUTCMinutes()};
}

function utc2locale(hours, minutes) {
  var date = new Date();
  date.setUTCHours(parseInt(hours), parseInt(minutes));
  return {hours:date.getHours(), minutes:date.getMinutes()};
}
