/* ================================================================
   FLOWSLAYER — Website Scripts
================================================================ */

'use strict';

/* ----------------------------------------------------------------
   HERO CANVAS — floating energy particles
---------------------------------------------------------------- */
(function initHeroCanvas() {
    const canvas = document.getElementById('heroCanvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');

    let W, H, particles;

    function resize() {
        W = canvas.width  = canvas.offsetWidth;
        H = canvas.height = canvas.offsetHeight;
    }

    class Particle {
        constructor(initial) { this.reset(initial); }

        reset(initial = false) {
            this.x        = Math.random() * (W || window.innerWidth);
            this.y        = initial
                ? Math.random() * (H || window.innerHeight)
                : (H || window.innerHeight) + 10;
            this.size     = Math.random() * 1.8 + 0.4;
            this.vy       = -(Math.random() * 0.65 + 0.15);
            this.vx       = (Math.random() - 0.5) * 0.25;
            this.opacity  = 0;
            this.maxOp    = Math.random() * 0.55 + 0.15;
            this.life     = 0;
            this.maxLife  = Math.random() * 280 + 180;
            /* Color: deep blue → cyan */
            const t   = Math.random();
            this.r    = Math.round(t * 80);
            this.g    = Math.round(160 + t * 72);
            this.b    = 255;
        }

        update() {
            this.x += this.vx;
            this.y += this.vy;
            this.life++;
            const ratio = this.life / this.maxLife;
            if (ratio < 0.12)       this.opacity = ratio / 0.12 * this.maxOp;
            else if (ratio > 0.65)  this.opacity = (1 - ratio) / 0.35 * this.maxOp;
            else                    this.opacity = this.maxOp;
            if (this.life >= this.maxLife || this.y < -10) this.reset();
        }

        draw() {
            ctx.save();
            ctx.globalAlpha = this.opacity;
            ctx.shadowBlur  = this.size * 7;
            ctx.shadowColor = `rgba(${this.r},${this.g},${this.b},0.85)`;
            ctx.fillStyle   = `rgb(${this.r},${this.g},${this.b})`;
            ctx.beginPath();
            ctx.arc(this.x, this.y, this.size, 0, Math.PI * 2);
            ctx.fill();
            ctx.restore();
        }
    }

    function init() {
        resize();
        particles = Array.from({ length: 130 }, (_, i) => new Particle(true));
        window.addEventListener('resize', resize, { passive: true });
    }

    function animate() {
        ctx.clearRect(0, 0, W, H);

        /* Subtle grid */
        ctx.strokeStyle = 'rgba(0,212,255,0.03)';
        ctx.lineWidth   = 1;
        const g = 64;
        for (let x = 0; x <= W; x += g) {
            ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, H); ctx.stroke();
        }
        for (let y = 0; y <= H; y += g) {
            ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(W, y); ctx.stroke();
        }

        /* Radial glow at center */
        const grad = ctx.createRadialGradient(W * 0.5, H * 0.52, 0, W * 0.5, H * 0.5, W * 0.55);
        grad.addColorStop(0, 'rgba(0,28,55,0.35)');
        grad.addColorStop(1, 'rgba(6,8,15,0)');
        ctx.fillStyle = grad;
        ctx.fillRect(0, 0, W, H);

        particles.forEach(p => { p.update(); p.draw(); });
        requestAnimationFrame(animate);
    }

    init();
    animate();
})();


/* ----------------------------------------------------------------
   FLOW WORD GLOW — particles rising from the "FLOW" title
---------------------------------------------------------------- */
(function initFlowWordGlow() {
    const canvas = document.getElementById('flowGlowCanvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    let W, H, bounds, particles;

    function calcBounds() {
        const word = document.getElementById('flow-word');
        const hero = document.getElementById('hero');
        if (!word || !hero) return;
        const h1 = word.closest('h1');
        if (!h1) return;

        /* Walk offsetParent chain from h1 up to hero to get canvas-relative Y.
           offsetTop on a block element (h1) is reliable; using span was not. */
        let top = 0;
        let el = h1;
        while (el && el !== hero) {
            top += el.offsetTop;
            el = el.offsetParent;
        }

        /* X still from viewport rects — reliable for inline text spans */
        const wr = word.getBoundingClientRect();
        const hr = hero.getBoundingClientRect();
        const lift = Math.round(h1.offsetHeight * 0.13);
        bounds = {
            x1: wr.left  - hr.left,
            x2: wr.right - hr.left,
            y1: top - lift,
            y2: top + h1.offsetHeight - lift,
        };
    }

    function resize() {
        W = canvas.width  = canvas.offsetWidth;
        H = canvas.height = canvas.offsetHeight;
        calcBounds();
    }

    class FP {
        reset(initial = false) {
            const b = bounds;
            this.x       = b ? b.x1 + Math.random() * (b.x2 - b.x1) : W / 2;
            this.y       = initial
                ? (b ? b.y1 + Math.random() * (b.y2 - b.y1) : H / 2)
                : (b ? b.y2 : H / 2);
            this.size    = Math.random() * 2.0 + 0.5;
            this.vy      = -(Math.random() * 0.85 + 0.25);
            this.vx      = (Math.random() - 0.5) * 0.35;
            this.opacity = 0;
            this.maxOp   = Math.random() * 0.65 + 0.25;
            this.life    = 0;
            this.maxLife = Math.random() * 160 + 100;
            const t = Math.random();
            this.r = Math.round(t * 30);
            this.g = Math.round(190 + t * 65);
            this.b = 255;
        }
        constructor(initial) { this.reset(initial); }
        update() {
            this.x += this.vx;
            this.y += this.vy;
            this.life++;
            const ratio = this.life / this.maxLife;
            if (ratio < 0.15)     this.opacity = (ratio / 0.15) * this.maxOp;
            else if (ratio > 0.6) this.opacity = ((1 - ratio) / 0.4) * this.maxOp;
            else                  this.opacity = this.maxOp;
            const top = bounds ? bounds.y1 - 100 : -10;
            if (this.life >= this.maxLife || this.y < top) this.reset();
        }
        draw() {
            ctx.save();
            ctx.globalAlpha = this.opacity;
            ctx.shadowBlur  = this.size * 8;
            ctx.shadowColor = `rgba(${this.r},${this.g},${this.b},0.9)`;
            ctx.fillStyle   = `rgb(${this.r},${this.g},${this.b})`;
            ctx.beginPath();
            ctx.arc(this.x, this.y, this.size, 0, Math.PI * 2);
            ctx.fill();
            ctx.restore();
        }
    }

    function animate() {
        ctx.clearRect(0, 0, W, H);
        particles.forEach(p => { p.update(); p.draw(); });
        requestAnimationFrame(animate);
    }

    document.fonts.ready.then(() => {
        resize();
        particles = Array.from({ length: 60 }, () => new FP(true));
        window.addEventListener('resize', resize, { passive: true });
        animate();
    });
})();


/* ----------------------------------------------------------------
   FLOW ORB CANVAS — pulsing max-tier orb
---------------------------------------------------------------- */
(function initFlowCanvas() {
    const canvas = document.getElementById('flowCanvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const W = 280, H = 280;
    const cx = W / 2, cy = H / 2;
    let t = 0;

    const rings = [
        { r: 96, speed:  0.009, count: 14, col: '0,255,159',  sz: 2.8 },
        { r: 114, speed: -0.006, count:  9, col: '0,212,255',  sz: 2.0 },
        { r: 76,  speed:  0.013, count: 18, col: '0,200,240',  sz: 1.6 },
    ];

    function draw() {
        ctx.clearRect(0, 0, W, H);

        /* Outer glow halos */
        [128, 108, 88].forEach((r, i) => {
            const pulse = 1 + Math.sin(t * 1.8 + i) * 0.25;
            ctx.strokeStyle = `rgba(0,255,159,${(0.045 - i * 0.013) * pulse})`;
            ctx.lineWidth   = 22 - i * 5;
            ctx.beginPath();
            ctx.arc(cx, cy, r, 0, Math.PI * 2);
            ctx.stroke();
        });

        /* Core orb */
        const coreGrad = ctx.createRadialGradient(cx, cy, 0, cx, cy, 52);
        coreGrad.addColorStop(0,   'rgba(0,255,195,0.90)');
        coreGrad.addColorStop(0.45,'rgba(0,212,255,0.55)');
        coreGrad.addColorStop(1,   'rgba(0,80,140,0)');
        ctx.fillStyle = coreGrad;
        ctx.beginPath();
        ctx.arc(cx, cy, 52, 0, Math.PI * 2);
        ctx.fill();

        /* Bright inner core */
        const innerGrad = ctx.createRadialGradient(cx, cy, 0, cx, cy, 18);
        innerGrad.addColorStop(0, 'rgba(255,255,255,0.95)');
        innerGrad.addColorStop(0.5,'rgba(0,255,195,0.80)');
        innerGrad.addColorStop(1, 'rgba(0,212,255,0)');
        ctx.fillStyle = innerGrad;
        ctx.beginPath();
        ctx.arc(cx, cy, 18, 0, Math.PI * 2);
        ctx.fill();

        /* Orbiting particles */
        rings.forEach(ring => {
            for (let i = 0; i < ring.count; i++) {
                const angle  = (i / ring.count) * Math.PI * 2 + t * ring.speed * 100;
                const wobble = Math.sin(t * 3.2 + i * 1.1) * 4;
                const rad    = ring.r + wobble;
                const px     = cx + Math.cos(angle) * rad;
                const py     = cy + Math.sin(angle) * rad;
                const alpha  = 0.35 + Math.sin(t * 2.4 + i * 0.6) * 0.3;

                ctx.save();
                ctx.shadowBlur  = 7;
                ctx.shadowColor = `rgba(${ring.col},0.85)`;
                ctx.fillStyle   = `rgba(${ring.col},${alpha})`;
                ctx.beginPath();
                ctx.arc(px, py, ring.sz, 0, Math.PI * 2);
                ctx.fill();
                ctx.restore();
            }
        });

        t += 0.018;
        requestAnimationFrame(draw);
    }

    draw();
})();


/* ----------------------------------------------------------------
   NAVBAR — scroll state + active link tracking
---------------------------------------------------------------- */
(function initNavbar() {
    const navbar   = document.getElementById('navbar');
    const links    = document.querySelectorAll('.nav-links a');
    const sections = [...document.querySelectorAll('section[id]')];

    function onScroll() {
        /* Frosted glass on scroll */
        navbar.classList.toggle('scrolled', window.scrollY > 28);

        /* Active link */
        let current = '';
        sections.forEach(s => {
            if (window.scrollY >= s.offsetTop - 110) current = s.id;
        });
        links.forEach(a => {
            a.classList.toggle('active', a.getAttribute('href') === `#${current}`);
        });
    }

    window.addEventListener('scroll', onScroll, { passive: true });
    onScroll(); /* run once on load */
})();


/* ----------------------------------------------------------------
   REVEAL — IntersectionObserver scroll animations
   Also triggers the Flow tier bar fills when they enter view.
---------------------------------------------------------------- */
(function initReveal() {
    const io = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (!entry.isIntersecting) return;

            const el       = entry.target;
            const siblings = [...el.parentElement.querySelectorAll('.reveal')];
            const idx      = siblings.indexOf(el);

            el.style.transitionDelay = `${idx * 75}ms`;
            el.classList.add('visible');

            /* Animate Flow bars that live inside this element */
            el.querySelectorAll('.tier-fill').forEach(bar => {
                bar.style.width = (bar.dataset.width || '0') + '%';
            });

            io.unobserve(el);
        });
    }, { threshold: 0.12 });

    document.querySelectorAll('.reveal').forEach(el => io.observe(el));
})();


/* ----------------------------------------------------------------
   SMOOTH SCROLL — offset for fixed navbar
---------------------------------------------------------------- */
document.querySelectorAll('a[href^="#"]').forEach(a => {
    a.addEventListener('click', e => {
        const id     = a.getAttribute('href');
        const target = document.querySelector(id);
        if (!target) return;
        e.preventDefault();
        window.scrollTo({
            top: target.offsetTop - 66,
            behavior: 'smooth',
        });
    });
});
