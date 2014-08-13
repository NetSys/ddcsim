#include "entities.h"

using std::cout;
using std::endl;

Entity::Ports::Ports() : port_to_entity_(), is_link_up_() {}

Entity::Entity() : ports_() {}

// TODO remove cout's after debugging

void Entity::Handle(Event* e) {
  cout << "Entity received event " << e->Description() << endl;
}

Switch::Switch() : is_up_(true) {}

void Switch::Handle(Event* e) {
  cout << "Switch received event " << e->Description() << endl;
}

void Switch::Handle(SwitchUp* e) { is_up_ = true; }

void Switch::Handle(SwitchDown* e) { is_up_ = false; }
