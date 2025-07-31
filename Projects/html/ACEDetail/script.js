document.addEventListener('DOMContentLoaded', function() {

    // --- True 3D Text Creation ---
    function create3DText() {
        const textContainer = document.getElementById("animated-name");
        if (!textContainer) return;

        const text = "ACE Detail";
        const depth = 6; // How many layers to create for the 3D effect
        textContainer.innerHTML = ''; // Clear existing content

        text.split('').forEach(char => {
            const letterContainer = document.createElement('div');
            letterContainer.classList.add('letter-container');

            if (char === ' ') {
                letterContainer.classList.add('space');
                textContainer.appendChild(letterContainer);
                return;
            }

            // NEW: Add a base layer that is NOT absolutely positioned.
            // This gives the container its width and height.
            const baseLayer = document.createElement('span');
            baseLayer.classList.add('base-layer');
            baseLayer.textContent = char;
            letterContainer.appendChild(baseLayer);

            // Create the visible 3D layers (absolutely positioned)
            for (let i = 1; i <= depth; i++) {
                const layer = document.createElement('span');
                layer.classList.add('layer');
                layer.textContent = char;
                layer.style.transform = `translateZ(-${i}px)`;
                layer.style.color = 'var(--accent-darker)';
                letterContainer.appendChild(layer);
            }

            // Create the front-most layer (absolutely positioned)
            const frontLayer = document.createElement('span');
            frontLayer.classList.add('layer');
            frontLayer.textContent = char;
            frontLayer.style.transform = 'translateZ(0)';
            frontLayer.style.color = 'var(--accent-color)';
            letterContainer.appendChild(frontLayer);

            textContainer.appendChild(letterContainer);
        });
    }

    create3DText();


    // --- 3D Spinning Container Animation ---
    const animatedElement = document.getElementById("animated-name");
    if (animatedElement) {
        let currentAngle = 0;
        let currentDirection = 0;
        const STEP = 1;
        const INTERVAL = 30; // Slightly slower for a smoother feel with the new text

        function spin() {
            if (currentDirection === 0) {
                if (currentAngle < 360) {
                    currentAngle += STEP;
                } else {
                    currentDirection = 1;
                    currentAngle -= STEP;
                }
            } else {
                if (currentAngle > 0) {
                    currentAngle -= STEP;
                } else {
                    currentDirection = 0;
                    currentAngle += STEP;
                }
            }
            // Apply rotation to the main H1 container
            animatedElement.style.transform = `rotate3d(0, 1, 0.1, ${currentAngle}deg)`;
        }
        setInterval(spin, INTERVAL);
    }

    // --- Mobile Navigation Toggle ---
    const navToggle = document.getElementById('nav-toggle');
    const navLinks = document.getElementById('nav-links');

    if (navToggle && navLinks) {
        // Toggle menu on hamburger click
        navToggle.addEventListener('click', () => {
            navLinks.classList.toggle('active');
            // Change icon to X when menu is open
            const icon = navToggle.querySelector('i');
            if (navLinks.classList.contains('active')) {
                icon.classList.remove('fa-bars');
                icon.classList.add('fa-times');
            } else {
                icon.classList.remove('fa-times');
                icon.classList.add('fa-bars');
            }
        });

        // Close menu when a link is clicked
        navLinks.querySelectorAll('a').forEach(link => {
            link.addEventListener('click', () => {
                navLinks.classList.remove('active');
                // Reset icon back to bars
                const icon = navToggle.querySelector('i');
                icon.classList.remove('fa-times');
                icon.classList.add('fa-bars');
            });
        });
    }
});
