let currentValue = '';
        let operator = '';
        let previousValue = '';

        function appendValue(value) {
            currentValue += value;
            updateDisplay(currentValue);
        }

        function setOperator(op) {
            if (currentValue === '') return;
            operator = op;
            previousValue = currentValue;
            currentValue = '';
        }

        function calculate() {
            if (previousValue === '' || currentValue === '' || operator === '') return;

            const num1 = parseFloat(previousValue);
            const num2 = parseFloat(currentValue);
            let result = 0;

            switch (operator) {
                case '+':
                    result = num1 + num2;
                    break;
                case '-':
                    result = num1 - num2;
                    break;
                case '*':
                    result = num1 * num2;
                    break;
                case '/':
                    result = num2 !== 0 ? num1 / num2 : 'Error';
                    break;
            }

            currentValue = result.toString();
            operator = '';
            previousValue = '';
            updateDisplay(currentValue);
        }

        function clearDisplay() {
            currentValue = '';
            operator = '';
            previousValue = '';
            updateDisplay('');
        }

        function updateDisplay(value) {
            document.getElementById('display').value = value;
        }

(function() {
  const windowTitle = 'Calculator';
  const customHTML = `
  <div class="calculator">
  <input type="text" class="display" id="display" disabled>
  <br><br>
  <div class="buttons">
      <button onclick="clearDisplay()" class="wide" style="margin-left: 75.4%; background: #fc2e20;">C</button>
      <br>
      <button onclick="appendValue('7')">7</button>
      <button onclick="appendValue('8')">8</button>
      <button onclick="appendValue('9')">9</button>
      <button onclick="setOperator('/')" style="background: #464033">/</button>
      <button onclick="appendValue('4')">4</button>
      <button onclick="appendValue('5')">5</button>
      <button onclick="appendValue('6')">6</button>
      <button onclick="setOperator('*')" style="background: #464033">*</button>
      <button onclick="appendValue('1')">1</button>
      <button onclick="appendValue('2')">2</button>
      <button onclick="appendValue('3')">3</button>
      <button onclick="setOperator('-')" style="background: #464033">-</button>
      <button onclick="appendValue('0')">0</button>
      <button onclick="appendValue('.')">.</button>
      <button onclick="calculate()" class="wide" style="background: #7e7c73">=</button>
      <button onclick="setOperator('+')" style="background: #464033">+</button>
  </div>
</div>
  `;
  
  // Create a window dynamically
  createWindow(windowTitle, customHTML, [
    { label: 'Close', action: closeWindow },
    { label: 'Minimize', action: minimizeWindow },
    { label: 'Maximize', action: maximizeWindow }
  ]);
})();