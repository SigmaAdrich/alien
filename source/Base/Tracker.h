#ifndef TRACKER_H
#define TRACKER_H

#include <boost/optional.hpp>
#include <vector>

using std::vector;

enum class TrackerUpdate {
	Yes, No
};

template<typename T>
class Tracker
{
private:
	boost::optional<T> _value;
	bool _isModified = false;

public:
	Tracker() = default;
	Tracker(T const &v) : _initialValue(v), _value(v) {}

	void init(T const& v)
	{
		_value = v;
		_isModified = false;
	}

	T* operator->() { return _value.get(); }
	T const* operator->() const { return _value->get; }

	bool isModified() const { return _isModified; }
	bool isInitialized() const { return _value.is_initialized(); }
	T const& getValue() const { return _value.get(); }
	T & getValue(TrackerUpdate update = TrackerUpdate::No)
	{
		if (update == TrackerUpdate::Yes) {
			_isModified = true;
		}
		return _value.get();
	}
	T const& getValueOr(T const& d) const { return _value.get_value_or(d); }
	T const& getValueOrDefault() const { return _value.get_value_or(T()); }
	Tracker& setValue(T const& v)
	{
		_value = v;
		_isModified = true;
		return *this;
	}
	void setAsUnmodified()
	{
		_isModified = false;
	}
	void reset()
	{
		_value.reset();
		_isModified = false;
	}
};

enum class TrackerElementState {
	Deleted, Modified, Unmodified, Added
};

template<typename T>
class TrackerElement
{
private:
	TrackerElementState _state = TrackerElementState::Unmodified;
	T _value;

public:
	TrackerElement() = delete;
	TrackerElement(T const &v) : _value(v) {}
	TrackerElement(T const &v, TrackerElementState s) : _state(s), _value(v) {}

	T const* operator->() const { return &_value; }
	T* operator->()
	{
		if (_state == TrackerElementState::Unmodified) {
			_state = TrackerElementState::Modified;
		}
		return &_value;
	}

	bool isDeleted() const { return _state == TrackerElementState::Deleted; }
	bool isModified() const { return _state == TrackerElementState::Modified; }
	bool isUnmodified() const { return _state == TrackerElementState::Unmodified; }
	bool isAdded() const { return _state == TrackerElementState::Added; }
	TrackerElement& setAsDeleted() { _state = TrackerElementState::Deleted; return *this; }
	TrackerElement& setAsAdded() { _state = TrackerElementState::Added; return *this; }
	TrackerElement& setAsModified() { _state = TrackerElementState::Modified; return *this; }
	TrackerElement& setAsUnmodified() { _state = TrackerElementState::Unmodified; return *this; }
	T const& getValue() const { return _value; }
	T & getValue(TrackerUpdate update = TrackerUpdate::No)
	{
		if (update == TrackerUpdate::Yes && _state == TrackerElementState::Unmodified) {
			_state = TrackerElementState::Modified;
		}
		return _value;
	}
	TrackerElement& setValue(T const& v) { _value = v; _state = TrackerElementState::Modified; return *this; }
};

template<typename T>
vector<T> getUndeletedElements(vector<TrackerElement<T>> const& elements)
{
	vector<T> result;
	for (auto const& element : elements) {
		if (!element.isDeleted()) {
			result.emplace_back(element.getValue());
		}
	}
	return result;
}

#endif // TRACKER_H
