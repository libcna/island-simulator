// Linked into both the deployment and QA builds. Do not defer the request to a
// polled game frame: browsers require transient activation from this key event.
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
    let held = false;
    let pending = false;
    const report = function (message) {
        const line = 'island-simulator: fullscreen ' + message;
        (Module['print'] || console.log)(line);
    };
    document.addEventListener('keydown', function (event) {
        // Keep gameplay Tab/Space from moving focus or scrolling the document.
        // Outside the focused/locked canvas, normal page keyboard access remains.
        if ((event.code === 'Tab' || event.code === 'Space') && Module['canvas'] &&
            (document.activeElement === Module['canvas'] || document.pointerLockElement === Module['canvas'])) {
            event.preventDefault();
        }
        if (event.code !== 'F11') return;
        event.preventDefault();
        if (!event.isTrusted || event.repeat || held) return;
        held = true;
        if (pending) return;
        try {
            let operation;
            if (document.fullscreenElement) {
                operation = document.exitFullscreen();
            } else {
                if (!document.fullscreenEnabled || !document.documentElement.requestFullscreen) {
                    report('unavailable: browser or embedding policy forbids fullscreen');
                    return;
                }
                operation = document.documentElement.requestFullscreen();
            }
            pending = true;
            Promise.resolve(operation).catch(function (error) {
                report('failed: ' + String(error));
            }).finally(function () { pending = false; });
        } catch (error) {
            pending = false;
            report('failed: ' + String(error));
        }
    }, true);
    document.addEventListener('keyup', function (event) {
        if (event.code === 'F11') held = false;
    }, true);
    window.addEventListener('blur', function () { held = false; });
    document.addEventListener('fullscreenchange', function () {
        // Observe real state, including Escape/browser-initiated exits.
        report(document.fullscreenElement ? 'on' : 'off');
    });
});
