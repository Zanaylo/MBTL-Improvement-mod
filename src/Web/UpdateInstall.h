#pragma once

#include "Web/Job.h"

#include <string>

class IDeviceListener;

namespace UpdateInstall
{
	struct Snapshot
	{
		Web::Job::Status job;
		bool busy;
		bool staged;
	};

	bool IsBusy();
	bool IsStaged();

	void Start();
	void Cancel();

	void OnFrame();
	IDeviceListener* Listener();

	void Read(Snapshot& out);
}
