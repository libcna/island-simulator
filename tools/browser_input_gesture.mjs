// A real test gesture, not a mutation of browser/game capture state. The caller supplies
// a bounded acknowledgement wait; always release, including an ambiguous transport failure
// while sending the press. Preserve the primary failure if diagnostics or release also fail.
export async function acknowledgedInputGesture({ press, acknowledge, release, inspect,
  evidence, now = Date.now }) {
  evidence.before = await inspect();
  const started = now();
  let failed = false;
  try {
    await press();
    const result = await acknowledge();
    evidence.acknowledgedAfterMilliseconds = now() - started;
    return result;
  } catch (error) {
    failed = true;
    evidence.failure = String(error);
    throw error;
  } finally {
    try { evidence.beforeRelease = await inspect(); }
    catch (error) { evidence.inspectionError = String(error); }
    try { await release(); }
    catch (error) {
      evidence.releaseError = String(error);
      if (!failed) throw error;
    } finally {
      evidence.heldMilliseconds = now() - started;
    }
  }
}
