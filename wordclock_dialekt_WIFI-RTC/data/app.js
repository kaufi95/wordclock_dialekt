const colorPicker = document.getElementById('color-picker');

const languageButtons = document.querySelectorAll('input[name="language"]');
const brightnessButtons = document.querySelectorAll('input[name="brightness"]');
const timeoutButtons = document.querySelectorAll('input[name="timeout"]');
const wifi_div = document.getElementById('wifi');

const ntp_input = document.getElementById('ntp');
const ssid_input = document.getElementById('ssid');
const pw_input = document.getElementById('pw');
const status_text = document.getElementById('status');

// hide the div container with id "wifi" if the ntp checkbox is unchecked
ntp_input.addEventListener('change', () => {
  wifi_div.style.display = ntp_input.checked ? 'block' : 'none';
});

if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener(
    'open',
    function (e) {
      console.log('Events Connected');
    },
    false
  );
  source.addEventListener(
    'error',
    function (e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log('Events Disconnected');
      }
    },
    false
  );

  source.addEventListener(
    'message',
    function (e) {
      console.log('message', e.data);
    },
    false
  );

  source.addEventListener(
    'status',
    function (e) {
      console.log('status', e.data);
      document.getElementById('status').innerHTML = e.data;
    },
    false
  );
}

document.addEventListener('DOMContentLoaded', () => {
  onLoad();
});

function updateUI(data) {
  updateColor(data.color);
  updateLanguage(data.language);
  updateBrightness(data.brightness);
  updateTimeout(data.timeout);
  updateSSID(data.ssid);
  updatePW('');
  updateStatus(data.status);
  updateNTP(data.ntp);
}

function updateColor(color) {
  colorPicker.value = rgb5652hex(color);
  colorPicker.select();
}

function updateLanguage(language) {
  languageButtons.forEach((button) => {
    if (button.value === language) {
      button.checked = true;
    }
  });
}

function updateBrightness(brightness) {
  brightnessButtons.forEach((button) => {
    if (button.value === brightness) {
      button.checked = true;
    }
  });
}

function updateTimeout(timeout) {
  timeoutButtons.forEach((button) => {
    if (button.value === timeout) {
      button.checked = true;
    }
  });
}

function updateSSID(ssid) {
  ssid_input.value = ssid;
}

function updatePW(pw) {
  pw_input.value = pw;
}

function updateStatus(status) {
  status_text.innerText = status;
}

function updateNTP(ntp) {
  ntp_input.checked = ntp !== '0';
  // wifi_div.style.display = ntp ? 'block' : 'none';
}

function getSelectedColor() {
  return hex2rgb565(colorPicker.value);
}

function getSelectedLanguage() {
  let selectedLanguage = '';
  languageButtons.forEach((button) => {
    if (button.checked) {
      selectedLanguage = button.value;
    }
  });
  return selectedLanguage;
}

function getSelectedBrightness() {
  let selectedBrightness = '';
  brightnessButtons.forEach((button) => {
    if (button.checked) {
      selectedBrightness = button.value;
    }
  });
  return selectedBrightness;
}

function getSelectedTimeout() {
  let selectedTimeout = '';
  timeoutButtons.forEach((button) => {
    if (button.checked) {
      selectedTimeout = button.value;
    }
  });
  return selectedTimeout;
}

function getSSID() {
  return ssid_input.value;
}

function getPW() {
  return pw_input.value;
}

function onLoad() {
  fetch('http://' + window.location.host + '/status')
    .then((response) => {
      if (!response.ok) {
        throw new Error('Network response was not ok');
      }
      return response.json();
    })
    .then((data) => {
      console.log('GET request successful');
      console.log(data);
      updateUI(data);
    })
    .catch((error) => console.error('Error sending GET request:', error));
}

function sendPostRequest() {
  const color = getSelectedColor();
  const language = getSelectedLanguage();
  const brightness = getSelectedBrightness();
  const timeout = getSelectedTimeout();
  const ntp = ntp_input.checked;
  const ssid = getSSID();
  const pw = getPW();

  const body = {
    datetime: new Date().valueOf().toString().slice(0, -3)
  };

  if (color !== '') {
    body.color = color;
  }

  if (language !== '') {
    body.language = language;
  }

  if (brightness !== '') {
    body.brightness = brightness;
  }

  if (timeout !== '') {
    body.timeout = timeout;
  }

  if (ntp) {
    body.ntp = ntp;
  }

  if (ntp && ssid !== '' && pw !== '') {
    body.ssid = ssid;
    body.pw = pw;
  }

  fetch('http://' + window.location.host + '/update', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(body)
  })
    .then((response) => {
      if (!response.ok) {
        throw new Error('Network response was not ok');
      }
      console.log('POST request sent successfully');
      console.log(body);
    })
    .catch((error) => console.error('Error sending POST request:', error));
}

const hex2rgb565 = (hex) => {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);

  const r5 = (r * 31) / 255;
  const g6 = (g * 63) / 255;
  const b5 = (b * 31) / 255;

  return (r5 << 11) | (g6 << 5) | b5;
};

const rgb5652hex = (rgb565) => {
  // Extract the red, green, and blue components from the 16-bit value
  let red5 = (rgb565 >> 11) & 0x1f;
  let green6 = (rgb565 >> 5) & 0x3f;
  let blue5 = rgb565 & 0x1f;

  // Scale the components back to the 8-bit range (0-255)
  let red = (red5 << 3) | (red5 >> 2); // Scale 5-bit red to 8-bit
  let green = (green6 << 2) | (green6 >> 4); // Scale 6-bit green to 8-bit
  let blue = (blue5 << 3) | (blue5 >> 2); // Scale 5-bit blue to 8-bit

  // Combine the components into a single 3-byte hex color code
  let hexColor = ((red << 16) | (green << 8) | blue)
    .toString(16)
    .padStart(6, '0');

  return `#${hexColor}`;
};
