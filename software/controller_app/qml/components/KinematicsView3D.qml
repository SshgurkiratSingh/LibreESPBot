import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    width: 400
    height: 400

    property real pitch: 0.0
    property real roll: 0.0
    property real yaw: 0.0

    // Throttle Canvas repaints to 30fps to prevent event loop flooding
    Timer {
        interval: 33
        running: true
        repeat: true
        onTriggered: canvas.requestPaint()
    }

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            var cx = width / 2;
            var cy = height / 2;
            var scale = Math.min(width, height) * 0.4; // Box size relative to viewport

            // Convert degrees to radians
            var p = root.pitch * Math.PI / 180.0;
            var r = -root.roll * Math.PI / 180.0;
            var y = -root.yaw * Math.PI / 180.0;

            // Precompute trig
            var cp = Math.cos(p), sp = Math.sin(p);
            var cr = Math.cos(r), sr = Math.sin(r);
            var cyaw = Math.cos(y), syaw = Math.sin(y);

            // Define 8 vertices of a rectangle (the rover chassis)
            // L = Length (X), W = Width (Y), H = Height (Z)
            var l = 0.8;
            var w = 0.5;
            var h = 0.2;
            
            var vertices = [
                {x:  l, y:  w, z:  h},
                {x:  l, y: -w, z:  h},
                {x: -l, y: -w, z:  h},
                {x: -l, y:  w, z:  h},
                {x:  l, y:  w, z: -h},
                {x:  l, y: -w, z: -h},
                {x: -l, y: -w, z: -h},
                {x: -l, y:  w, z: -h}
            ];

            var projected = [];

            // 3D Rotation Math
            for (var i = 0; i < vertices.length; i++) {
                var vx = vertices[i].x;
                var vy = vertices[i].y;
                var vz = vertices[i].z;

                // Rotate around Y-axis (Yaw)
                var x1 = vx * cyaw + vz * syaw;
                var y1 = vy;
                var z1 = -vx * syaw + vz * cyaw;

                // Rotate around Z-axis (Pitch)
                var x2 = x1 * cp - y1 * sp;
                var y2 = x1 * sp + y1 * cp;
                var z2 = z1;

                // Rotate around X-axis (Roll)
                var x3 = x2;
                var y3 = y2 * cr - z2 * sr;
                var z3 = y2 * sr + z2 * cr;

                // Orthographic Projection
                projected.push({
                    x: cx + x3 * scale,
                    y: cy + y3 * scale
                });
            }

            // Draw edges
            var edges = [
                [0, 1], [1, 2], [2, 3], [3, 0], // Top face
                [4, 5], [5, 6], [6, 7], [7, 4], // Bottom face
                [0, 4], [1, 5], [2, 6], [3, 7]  // Connecting struts
            ];

            ctx.lineWidth = 3;
            ctx.lineJoin = "round";
            
            // Draw background grid lines (optional decorative)
            ctx.strokeStyle = "#333333";
            ctx.beginPath();
            for(var i=0; i<10; i++) {
                ctx.moveTo(0, i * height/10); ctx.lineTo(width, i * height/10);
                ctx.moveTo(i * width/10, 0); ctx.lineTo(i * width/10, height);
            }
            ctx.stroke();

            // Draw wireframe
            for (var i = 0; i < edges.length; i++) {
                var p1 = projected[edges[i][0]];
                var p2 = projected[edges[i][1]];
                
                // Color the front face differently to indicate heading
                if (edges[i][0] === 0 || edges[i][1] === 0 || edges[i][0] === 1 || edges[i][1] === 1) {
                    ctx.strokeStyle = "#00FF00"; // Front edges
                } else {
                    ctx.strokeStyle = "#00E676"; // Main body edges
                }

                ctx.beginPath();
                ctx.moveTo(p1.x, p1.y);
                ctx.lineTo(p2.x, p2.y);
                ctx.stroke();
            }
            
            // Draw vertices
            ctx.fillStyle = "white";
            for (var i = 0; i < projected.length; i++) {
                ctx.beginPath();
                ctx.arc(projected[i].x, projected[i].y, 4, 0, 2 * Math.PI);
                ctx.fill();
            }
        }
    }
    
    // Data Overlay
    ColumnLayout {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 20
        spacing: 5
        
        Text {
            text: "3D WIREFRAME KINEMATICS"
            color: "#00E676"
            font.bold: true
            font.pixelSize: 16
            Layout.alignment: Qt.AlignHCenter
        }
        Text {
            text: "P: " + root.pitch.toFixed(1) + "° | R: " + root.roll.toFixed(1) + "° | Y: " + root.yaw.toFixed(1) + "°"
            color: "white"
            font.pixelSize: 14
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
