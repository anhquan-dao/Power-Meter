// Extend the graph with the data from the server
function updateGraph(result) {
    time_data       = result_.map(d => sec2dt(d[0]));
    current_data    = result_.map(d => d[1]);
    voltage_data    = result_.map(d => d[2]);

    Plotly.extendTraces('power_chart',{
        y:[voltage_data, current_data],
        x:[time_data, time_data],
    }, [0, 1]);
}
// Function to send periodically to get new measurements from the server
function requestData() {
    // Ajax call to get the Data from Server
    var requests = $.get('/esp32_post');
    
    // Wait for response, and update the graph accordingly
    var tm = requests.done(function (result) {
        if(result.length != 0) updateGraph(result);
    });
}
setInterval(function(){requestData()}, 1000);

// Substitution action for form element
function submitForm(e) {   
    // Prevent the default action of requesting/load new url
    e.preventDefault()
    
    var formID = e.target.id;
    var form = document.getElementById(formID);
    var data = new FormData(form);

    // Ajax call to submit the form to server
    $.ajax({
        type: form.method,
        url: form.action,
        data: $('#' + formID +'').serialize(),
    });
}