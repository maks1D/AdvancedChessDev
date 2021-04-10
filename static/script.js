const events = {
	connected: 0,
	ping: 1
};

var websocket = null;
var timeout = 1000;

var connect = () =>
{
	websocket = new WebSocket(`ws://${window.location.hostname}/websocket/`);

	websocket.onmessage = (event) =>
	{
		let data = JSON.parse(event.data);

		switch (data.type)
		{
			case events.connected:
			{
				timeout = 1000;
				document.getElementById("loader").style.display = "none";
				document.getElementById("container").style.display = "flex";

				break;
			}
		}
	};

	websocket.onclose = () =>
	{
		document.getElementById("loader").style.display = "block";
		document.getElementById("container").style.display = "none";

		timeout = Math.min(2 * timeout, 32000);
		setTimeout(connect, timeout);
	};
};

setInterval(() =>
{
	if (websocket !== null && websocket.readyState === WebSocket.OPEN)
	{
		websocket.send("\x00");
    }
}, 5000);

connect();