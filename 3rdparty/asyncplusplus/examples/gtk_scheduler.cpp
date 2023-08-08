// Copyright (c) 2015 Amanieu d'Antras
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <async++.h>
#include <gtk/gtk.h>
#include <iostream>
#include <chrono>
#include <string>

// Scheduler implementation
class gtk_scheduler_impl {
	// Get the task from the void* and execute it in the UI thread
	static gboolean callback(void* p)
	{
		async::task_run_handle::from_void_ptr(p).run();
		return FALSE;
	}

public:
	// Convert a task to void* and send it to the gtk main loop
	void schedule(async::task_run_handle t)
	{
		g_idle_add(callback, t.to_void_ptr());
	}
};

// Scheduler to run a task in the gtk main loop
gtk_scheduler_impl& gtk_scheduler()
{
	static gtk_scheduler_impl sched;
	return sched;
}

// In order to ensure the UI is always responsive, you can disallow blocking
// calls in the UI thread. Note that the wait handler isn't called when the
// result of a task is already available, so you can still call .get() on a
// completed task. This is completely optional and can be omitted if you don't
// need it.
void gtk_wait_handler(async::task_wait_handle)
{
	std::cerr << "Error: Blocking wait in UI thread" << std::endl;
	std::abort();
}

// Thread which increments the label value every ms
void label_update_thread(GtkLabel *label)
{
	int counter = 0;
	while (true) {
		// Sleep for 1ms
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		counter++;

		// Update the label contents in the UI thread
		async::spawn(gtk_scheduler(), [label, counter] {
			gtk_label_set_text(label, std::to_string(counter).c_str());
		});
	}
}

int main(int argc, char *argv[])
{
	// Initialize GTK
	gtk_init(&argc, &argv);

	// Set wait handler on GTK thread to disallow waiting for tasks
	async::set_thread_wait_handler(gtk_wait_handler);

	// Create a window with a label
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	GtkLabel *label = GTK_LABEL(gtk_label_new("-"));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(label));
	gtk_widget_show_all(window);

	// Start a secondary thread to update the label
	std::thread(label_update_thread, label).detach();

	// Go to GTK main loop
	gtk_main();
}
