document.addEventListener('DOMContentLoaded', function () {
  // Function to update date and time in the UI
  function updateDateTime() {
    document.getElementById('datetime').textContent = new Date().toLocaleString(
      'de-DE'
    );
  }

  // Update date and time every second
  setInterval(updateDateTime, 500);

  // Function to send HTTP GET request
  function sendGETRequest(color) {
    const language = getSelectedLanguage();

    if (language === '') {
      alert('Please select a language');
      return;
    }

    const body = {
      datetime: new Date().valueOf().toString().slice(0, -3),
      color: color,
      language: language
    };

    fetch('http://192.168.4.1/update', {
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
      })
      .catch((error) => console.error('Error sending POST request:', error));
  }

  const colorBoxes = document.querySelectorAll('.color-box');
  colorBoxes.forEach((box) => {
    box.addEventListener('click', function () {
      sendGETRequest(this.style.backgroundColor);
    });
  });

  function getSelectedLanguage() {
    let selectedLanguage = '';
    const languageRadioButtons = document.querySelectorAll(
      'input[name="language"]'
    );
    languageRadioButtons.forEach((button) => {
      if (button.checked) {
        selectedLanguage = button.value;
      }
    });
    return selectedLanguage;
  }
});
