#include "inferencelifecycle.h"
#include <cassert>

int main()
{
    InferenceLifecycle lifecycle;
    assert(lifecycle.requestStart(false) == InferenceLifecycle::Action::StartNow);
    assert(lifecycle.state() == InferenceLifecycle::State::Starting);
    assert(lifecycle.requestStart(true) == InferenceLifecycle::Action::Ignore);
    lifecycle.markFirstFrame();
    assert(lifecycle.state() == InferenceLifecycle::State::Running);
    lifecycle.requestStop();
    assert(lifecycle.state() == InferenceLifecycle::State::Stopping);
    lifecycle.markProcessExited();
    assert(lifecycle.state() == InferenceLifecycle::State::Idle);

    lifecycle.requestStart(false);
    lifecycle.markFailure();
    assert(lifecycle.state() == InferenceLifecycle::State::Error);
    assert(lifecycle.requestStart(true) == InferenceLifecycle::Action::QueueAfterCleanup);
    lifecycle.markProcessExited();
    assert(lifecycle.takeQueuedRestart());
    assert(!lifecycle.takeQueuedRestart());
}
