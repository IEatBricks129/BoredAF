(function() {
    const savedTheme = localStorage.getItem('theme');
    let shouldDark = false;
    if (savedTheme === 'dark') {
        shouldDark = true;
    } else if (savedTheme === 'light') {
        shouldDark = false;
    } else {
        const hour = new Date().getHours();
        shouldDark = (hour >= 18 || hour < 6);
    }
    if (shouldDark) {
        document.documentElement.classList.add('darkmode-preload');
        document.body.classList.add('darkmode');
    } else {
        document.documentElement.classList.remove('darkmode-preload');
        document.body.classList.remove('darkmode');
    }
})();

document.addEventListener('DOMContentLoaded', () => {
    let sixPresses = [];
    document.addEventListener('keydown', (e) => {
        if (e.key === '6') {
            const now = Date.now();
            sixPresses.push(now);
            sixPresses = sixPresses.filter(ts => now - ts <= 6000);
            if (sixPresses.length >= 6) {
                document.body.classList.add('wingdings-mode');
                sixPresses = [];
            }
        } else {
            sixPresses = [];
        }
    });

    function setTheme(mode) {
        if (mode === 'dark') {
            document.body.classList.add('darkmode');
        } else {
            document.body.classList.remove('darkmode');
        }
        document.body.style.display = 'none';
        document.body.offsetHeight;
        document.body.style.display = '';
    }

    function getPreferredTheme() {
        return localStorage.getItem('theme');
    }

    function setPreferredTheme(mode) {
        localStorage.setItem('theme', mode);
    }

    document.documentElement.classList.remove('darkmode-preload');

    const savedTheme = getPreferredTheme();
    if (savedTheme === 'dark' || savedTheme === 'light') {
        setTheme(savedTheme);
    } else {
        const hour = new Date().getHours();
        setTheme(hour >= 18 || hour < 6 ? 'dark' : 'light');
    }

    const themeToggle = document.getElementById('theme-toggle');
    if (themeToggle) {
        themeToggle.addEventListener('click', () => {
            const isDark = document.body.classList.toggle('darkmode');
            setPreferredTheme(isDark ? 'dark' : 'light');
            document.body.style.display = 'none';
            document.body.offsetHeight;
            document.body.style.display = '';
        });
    }

    document.body.classList.add('js-ready');
    document.body.classList.add('fade-init');
    requestAnimationFrame(() => {
        document.body.classList.remove('fade-init');
    });

    const navLinks = document.querySelectorAll('nav ul li a, .project-item .button, .back-to-projects .button, .read-more-link');

    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            const targetHref = this.getAttribute('href');
            if (e.button !== 0 || e.metaKey || e.ctrlKey || e.shiftKey || e.altKey) return;
            if (this.target === '_blank') return;

            const isSamePageAnchor = targetHref && targetHref.startsWith('#') && this.pathname === window.location.pathname;
            const isInternalLink = targetHref && this.origin === window.location.origin && !isSamePageAnchor;

            if (isSamePageAnchor) {
                e.preventDefault();
                this.classList.add('is-animating');
                this.addEventListener('animationend', () => {
                    this.classList.remove('is-animating');
                    const targetId = targetHref.substring(1);
                    const targetElement = document.getElementById(targetId);
                    if (targetElement) {
                        targetElement.scrollIntoView({ behavior: 'smooth' });
                    }
                }, { once: true });
            } else if (isInternalLink) {
                e.preventDefault();
                this.classList.add('is-animating');
                this.addEventListener('animationend', () => {
                    this.classList.remove('is-animating');
                    document.body.classList.remove('js-ready');
                    document.body.classList.add('fade-out');
                    setTimeout(() => {
                        window.location.href = targetHref;
                    }, 350);
                }, { once: true });
            }
        });
    });
});