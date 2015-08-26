# gnumeric-plugin-pwhtml
Custom version of CSV explort plugin for Gnumeric

Example input data (saved as example.xls):

Item     | Value
-------- | ---
Computer | $1,600.00
Phone    | $12.00
Pipe     | $1<sup>00</sup>

The command "ssconvert example.xls example.pwhtml" will produce CSV data:

    "Item","Item","Item","Value","Value","Value",
    "Computer","Computer","Computer","1600","$1 600,00","$1 600,00",
    "Phone","Phone","Phone","12","$12,00","$12,00",
    "Pipe","Pipe","Pipe","$100","$100","$1<sup>00</sup>",
    ---

If we import the resulting CSV data back in spreadsheet application:

Item     |Item     |Item     | Value     | Value      | Value      
-------- |-------- |-------- | --------- | ---------- | --------- 
Computer |Computer |Computer | 1600      | $1 600.00 | $1 600.00
Phone    |Phone    |Phone    | 12        | $12,00    | $12,00
Pipe     |Pipe     |Pipe     | $100     | $100      | $1&lt;sup&gt;00&lt;/sup&gt;

What happened? Each row from original file produced a row in CSV file. But each value is repeated three times. And there are slight but important differences between three versions.

* First copy contains the "raw" value with no formatting applied
* Second copy is formatted value. For example, if the cell has numeric value, formatted currency format, this value will contain currency symbol.
* Third copy is same formatted value, possibly with some styling information expressed as HTML tags: &lt;i&gt;, &lt;u&gt;, &lt;b&gt;, &lt;sup&gt;, &lt;sub&gt;

This is useful when you have an app, script or service that wants to consume data from .xls or .xlsx files.
 Your consumer needs both raw and formatted values (raw values for sorting and comparison, formatted values for display). Formulas from the spreadsheet need to be properly evaluated. And you want to offload the complexity of XLS handling to someone else (gnumeric in this case).

## Notes
pwhtml plugin processes all sheets from input file. It delimits them with a "---" line in CSV file.

pwhtml deals with merged cells in a special way. It treats merged regions as if each cell inside the region has the same value as top left corner cell. 

Disclaimer: I have no experience with C, and hacked this together using the tried and true "monkey see–monkey do" development methodology. Use at your own risk. 
