#include <cassert>

#include "latest_value_slot.hpp"

int main()
{
    LatestValueSlot<int> slot;
    int value = -1;

    assert(!slot.takeLatest(value));

    slot.publish(1);
    slot.publish(2);
    slot.publish(3);

    assert(slot.takeLatest(value));
    assert(value == 3);
    assert(!slot.takeLatest(value));

    slot.publish(4);
    slot.clear();
    assert(!slot.takeLatest(value));

    return 0;
}
