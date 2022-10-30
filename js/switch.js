const switchURL = 'http://localhost:8080/switch';

const switchOn = function() {
    const command = { "type": "on" };
    fetch(switchURL, {
            method: 'POST',
            body: JSON.stringify(command)
        }).then(res => res.json())
        .then(res => console.log(res));
}

const switchOff = function() {
    const command = { "type": "off" };
    fetch(switchURL, {
            method: 'POST',
            body: JSON.stringify(command)
        }).then(res => res.json())
        .then(res => console.log(res));
}