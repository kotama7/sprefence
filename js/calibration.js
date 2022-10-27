const pictureURL = 'http://localhost:8080/camera.jpg';
const setURL = 'http://localhost:8080/sethazard';


function calibration() {
    document.getElementById('init').innerHTML = '<canvas id="cv" height="480px" width="640px"></canvas>';
    let cvs = document.getElementById("cv");
    let ctx = cvs.getContext('2d');
    let img = new Image();
    img.onload = (e) => {
        ctx.drawImage(img, 0, 0);
    }
    img.src = pictureURL;
    let count = 0;
    let loc_ls = [];

    cvs.addEventListener("click", (e) => {
        const rect = e.target.getBoundingClientRect()
        const x = e.clientX - rect.left
        const y = e.clientY - rect.top
        ctx.beginPath();
        ctx.arc(x, y, 10, 0, Math.PI * 2, true);
        ctx.fill();
        ctx.closePath();
        console.log("called");
        ctx.fillStyle = "black";
        loc_ls[count] = [x / 640, y / 480];
        count += 1;
        console.log(`${x}:${y}`);
        if (count === 4) {
            let obj = { 'points': [] };
            for (i = 0; i < 4; i++) {
                obj.points[i] = { 'x': loc_ls[i][0], 'y': loc_ls[i][1] };
            }

            setTimeout(() => {
                if (window.confirm('以上の点でよろしいですか？')) {
                    fetch(setURL, {
                            method: 'POST',
                            body: JSON.stringify(obj)
                        }).then(res => res.json())
                        .then(res => console.log(res));
                    window.location.href = "./index.html"
                } else {
                    loc_ls = [];
                    count = 0;
                }
            }, 100);
        }
    });
}

window.onload = calibration()