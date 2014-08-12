#include "entities.h"

using std::cout;
using std::endl;

Entity::Entity() {}

// TODO remove cout's after debugging

void Entity::Handle(Event* e) {
  cout << "Entity received event " << e->Description() << endl;
}

Switch::Switch() {}

void Switch::Handle(Event* e) {
  cout << "Switch received event " << e->Description() << endl;
}

void Switch::Handle(SwitchUp* e) {
  cout << "Switch received event " << e->Description() << endl;
}

void Switch::Handle(SwitchDown* e) {
  cout << "Switch received event " << e->Description() << endl;
}
