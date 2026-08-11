#ifndef INFERENCELIFECYCLE_H
#define INFERENCELIFECYCLE_H

class InferenceLifecycle
{
public:
    enum class State { Idle, Starting, Running, Stopping, Error };
    enum class Action { StartNow, QueueAfterCleanup, Ignore };

    Action requestStart(bool processAlive)
    {
        if (m_state == State::Error && processAlive) {
            m_restartQueued = true;
            return Action::QueueAfterCleanup;
        }
        if ((m_state == State::Idle || m_state == State::Error) && !processAlive) {
            m_state = State::Starting;
            return Action::StartNow;
        }
        return Action::Ignore;
    }
    void markFirstFrame() { if (m_state == State::Starting) m_state = State::Running; }
    void requestStop() { if (m_state != State::Idle) m_state = State::Stopping; }
    void markFailure() { m_state = State::Error; m_restartQueued = false; }
    void markProcessExited() { if (m_state != State::Error) m_state = State::Idle; }
    bool takeQueuedRestart() { const bool value = m_restartQueued; m_restartQueued = false; return value; }
    State state() const { return m_state; }
private:
    State m_state = State::Idle;
    bool m_restartQueued = false;
};
#endif
