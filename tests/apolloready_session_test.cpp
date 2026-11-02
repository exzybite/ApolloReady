#include "src/platform/windows/apolloready_session.h"
#include <cassert>
#include <iostream>
#include <thread>

int main() {
  using namespace apolloready;
  session_store store;
  assert(!store.read().active);
  store.begin(100, 123);
  auto first = store.read();
  assert(first.status == phase::waiting);
  assert(first.app_id == 123);
  assert(!store.track_launch(first.generation, 0));
  assert(store.track_launch(first.generation, 20));
  assert(store.read().launched_process_id == 20);
  assert(!store.publish(first.generation, phase::ready));
  assert(!store.track_launch(first.generation, 99));
  assert(!store.bind(first.generation, 10, 20, 99));  // old process
  assert(store.bind(first.generation, 10, 20, 101));
  assert(!store.bind(first.generation, 11, 21, 102));  // cannot replace a source
  assert(store.publish(first.generation, phase::ready));
  assert(store.publish(first.generation, phase::closing));
  assert(!store.publish(first.generation, phase::ready));
  assert(!store.bind(first.generation, 12, 22, 103));
  store.begin(200, 456);
  auto second = store.read();
  assert(second.generation != first.generation && second.window == 0);
  assert(second.app_id == 456);
  assert(second.launched_process_id == 0);
  assert(!store.track_launch(first.generation, 99));
  assert(store.track_launch(second.generation, 21));
  assert(store.read().launched_process_id == 21);
  assert(!store.publish(first.generation, phase::closing));  // stale capture/callback
  assert(!store.bind(first.generation, 99, 99, 201));
  assert(store.bind(second.generation, 10, 20, 201));  // same identifiers, new epoch
  std::thread stale([&]() {
    for (int i = 0; i < 10000; ++i) store.publish(first.generation, phase::closing);
  });
  for (int i = 0; i < 10000; ++i) assert(store.publish(second.generation, phase::ready));
  stale.join();
  assert(store.read().status == phase::ready);
  store.end();
  assert(!store.read().active && store.read().status == phase::closing);
  assert(!store.publish(second.generation, phase::ready));
  assert(!store.bind(second.generation, 30, 40, 300));
  assert(!store.track_launch(second.generation, 99));
  std::cout << "PASS: source identity, closing latch, launch epochs and stale concurrent publications\n";
}
