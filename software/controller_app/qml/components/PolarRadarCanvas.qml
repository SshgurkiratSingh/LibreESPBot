import QtQuick 2.15

Canvas {
    id: radarCanvas
    property var points: [] // Array of {x, y} objects from RadarPointCloud
    property int maxDistance: 2000 // 200 cm

    onPaint: {
        var ctx = getContext("2d");
        ctx.clearRect(0, 0, width, height);
        
        var centerX = width / 2;
        var centerY = height / 2; // Center of the 360 radar
        var maxRadius = Math.min(width, height) / 2;
        
        // Draw distance rings
        ctx.strokeStyle = "#00E5FF";
        ctx.lineWidth = 1;
        ctx.globalAlpha = 0.3;
        
        var ringDistances = [500, 1000, 1500, 2000]; // mm
        for (let i = 0; i < ringDistances.length; ++i) {
            let r = (ringDistances[i] / maxDistance) * maxRadius;
            ctx.beginPath();
            ctx.arc(centerX, centerY, r, 0, 2 * Math.PI);
            ctx.stroke();
            
            // Draw distance label
            ctx.fillStyle = "#00E5FF";
            ctx.font = "10px sans-serif";
            ctx.fillText(ringDistances[i] + "mm", centerX + 5, centerY - r - 5);
        }
        
        ctx.globalAlpha = 1.0;
        
        // Plot points
        for (let i = 0; i < points.length; ++i) {
            let p = points[i];
            
            // Map mm to pixels (p.x and p.y can now be negative/positive covering all 4 quadrants)
            let px = centerX + (p.x / maxDistance) * maxRadius;
            let py = centerY - (p.y / maxDistance) * maxRadius;
            
            // Calculate distance for color coding
            let dist = Math.sqrt(p.x * p.x + p.y * p.y);
            
            if (dist < 200) {
                ctx.fillStyle = "#FF1744"; // Red < 20cm
            } else if (dist < 500) {
                ctx.fillStyle = "#FF9100"; // Orange < 50cm
            } else {
                ctx.fillStyle = "#00E676"; // Green > 50cm
            }
            
            ctx.beginPath();
            ctx.arc(px, py, 4, 0, 2 * Math.PI);
            ctx.fill();
        }
    }
    
    onPointsChanged: {
        requestPaint();
    }
}
