const colorPicker = document.getElementById('color-picker');

const languageButtons = document.querySelectorAll('input[name="language"]');
const brightnessButtons = document.querySelectorAll('input[name="brightness"]');
const termination_input = document.getElementById('termination');

document.addEventListener('DOMContentLoaded', () => {
  onLoad();
});

function updateUI(data) {
  updateColor(data.color);
  updateLanguage(data.language);
  updateBrightness(data.brightness);
  updateTermination(data.termination);
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
    if (button.value === brightness.toString()) {
      button.checked = true;
    }
  });
}

function updateTermination(termination) {
  termination_input.checked = termination;
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

function getTermination() {
  return termination_input.checked;
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
  const termination = getTermination();

  const body = {
    datetime: Math.round(new Date().getTime() / 1000),
    color: color,
    language: language,
    brightness: brightness,
    termination: termination
  };

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
