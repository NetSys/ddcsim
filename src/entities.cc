#include "entities.h"

using std::cout;
using std::endl;

Entity::Entity() {}

// TODO remove cout's after debugging

void Entity::Handle(Event* e) {
  cout << "Entity received event " << e->Description() << endl;
}

Router::Router() {}

void Router::Handle(Event* e) {
  cout << "Router received event " << e->Description() << endl;
}

void Router::Handle(RouterUpEvent* e) {
  cout << "Router received event " << e->Description() << endl;
}

void Router::Handle(RouterDownEvent* e) {
  cout << "Router received event " << e->Description() << endl;
}
