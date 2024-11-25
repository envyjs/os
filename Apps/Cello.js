let clipboard = null;
        let currentCell = null;

        // Adds a new row to the spreadsheet
        function addRow() {
            let table = document.getElementById('spreadsheet').getElementsByTagName('tbody')[0];
            let newRow = table.insertRow();
            let columnCount = table.rows[0].cells.length;

            for (let i = 0; i < columnCount; i++) {
                let newCell = newRow.insertCell(i);
                let input = document.createElement('input');
                newCell.appendChild(input);
            }
        }

        // Adds a new column to the spreadsheet
        function addColumn() {
            let table = document.getElementById('spreadsheet');
            let rows = table.rows;

            // Add new header cell
            let headerCell = document.createElement('th');
            rows[0].appendChild(headerCell);

            // Add input cells in each row
            for (let i = 1; i < rows.length; i++) {
                let newCell = rows[i].insertCell(rows[i].cells.length);
                let input = document.createElement('input');
                newCell.appendChild(input);
            }
        }

        // Function to convert the table data to CSV format
        function tableToCSV() {
            let table = document.getElementById('spreadsheet');
            let rows = table.rows;
            let csv = [];

            for (let i = 0; i < rows.length; i++) {
                let row = [];
                let cells = rows[i].cells;
                for (let j = 0; j < cells.length; j++) {
                    let cell = cells[j].querySelector('input') ? cells[j].querySelector('input').value : '';
                    row.push(cell);
                }
                csv.push(row.join(','));
            }

            return csv.join('\n');
        }

        // Function to trigger CSV download
        function saveToCSV() {
            let csvData = tableToCSV();
            let blob = new Blob([csvData], { type: 'text/csv' });
            let link = document.createElement('a');
            link.href = URL.createObjectURL(blob);
            link.download = 'spreadsheet.csv';
            link.click();
        }

        // Function to load CSV data into the table
        function loadCSV(event) {
            const file = event.target.files[0];
            if (!file) return;

            const reader = new FileReader();
            reader.onload = function(e) {
                const contents = e.target.result;
                const rows = contents.split('\n');
                const table = document.getElementById('spreadsheet').getElementsByTagName('tbody')[0];
                table.innerHTML = ''; // Clear the existing table

                // Loop through each row in the CSV
                rows.forEach((row, index) => {
                    const cells = row.split(',');
                    const tableRow = table.insertRow();

                    // Loop through each cell in the row and create an input element
                    cells.forEach(cell => {
                        const tableCell = tableRow.insertCell();
                        const input = document.createElement('input');
                        input.value = cell.trim(); // Populate the input with the cell value
                        tableCell.appendChild(input);
                    });

                    // Add extra columns for missing cells (if the row has fewer columns than the header)
                    if (index === 0) {
                        const columnCount = tableRow.cells.length;
                        while (table.rows[0].cells.length < columnCount) {
                            let headerCell = document.createElement('th');
                            table.rows[0].appendChild(headerCell);
                        }
                    }
                });
            };

            reader.readAsText(file); // Read the file as text
        }

        // Cut functionality (store the value in clipboard)
        function cut() {
            if (currentCell) {
                clipboard = currentCell.value;
                currentCell.value = ''; // Clear the selected cell
            }
        }

        // Copy functionality (store the value in clipboard)
        function copy() {
            if (currentCell) {
                clipboard = currentCell.value;
            }
        }

        // Paste functionality (paste clipboard content into the selected cell)
        function paste() {
            if (currentCell && clipboard !== null) {
                currentCell.value = clipboard;
            }
        }

        // Event listener to track cell selection and update `currentCell`
        document.querySelectorAll('table input').forEach(input => {
            input.addEventListener('focus', function() {
                currentCell = input;
            });
        });

(function() {
  const windowTitle = 'Cello';
  const customHTML = `
  <div class="spreadsheet-container">
        <div class="ribbon cl">
            <button onclick="paste()" style="height: 68px; max-width: 60px !important;">Paste</button>
            <button onclick="cut()" style="width: 56px; height: 30px;">Cut</button>
            <button onclick="copy()" style="height: 30px; position: absolute; top: 47px; left: 79px;">Copy</button>
            <button onclick="addRow()" style="height: 30px; width: 101px;">Add Row</button>
            <button onclick="addColumn()" style="height: 30px; position: absolute; top: 47px; left: 144px;">Add Column</button>
            <button style="height: 30px"><label for="fileInput">Open</label></button>
            <button onclick="saveToCSV()" style="height: 30px; width: 57px; position: absolute; top: 47px; left: 256px;">Save</button>
            <input type="file" id="fileInput" class="fis" onchange="loadCSV(event)" />
        </div>
        <table id="spreadsheet">
            <thead>
                <tr>
                    <th></th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><input type="text" class="cls"/></td>
                </tr>
            </tbody>
        </table>
    </div>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, [
    { label: 'Close', action: closeWindow },
    { label: 'Minimize', action: minimizeWindow },
    { label: 'Maximize', action: maximizeWindow }
  ]);
})();

notifier.create('Hello from the Example App', 'info', 5000);