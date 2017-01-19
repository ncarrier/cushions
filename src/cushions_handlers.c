#define _GNU_SOURCE
#include <dirent.h>
#include <dlfcn.h>
#include <fnmatch.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cushions_handler.h"
#include "cushions_handlers.h"
#define LOG_TAG cushions_handlers
#include "log.h"

#define MAX_HANDLER_PLUGINS 20
#ifndef CUSHIONS_DEFAULT_HANDLERS_PATH
#define CUSHIONS_DEFAULT_HANDLERS_PATH "/usr/lib/cushions_handlers"
#endif /* CUSHIONS_DEFAULT_HANDLERS_PATH */
/* allows to add a path in where additional handlers will be searched */
#define CUSHIONS_HANDLERS_ENV "CUSHIONS_HANDLERS_PATH"

#define PLUGINS_MATCHING_PATTERN "*.so"

static void *handler_plugins[MAX_HANDLER_PLUGINS];
static bool plugins_initialized;

static int pattern_filter(const struct dirent *d)
{
	return fnmatch(PLUGINS_MATCHING_PATTERN, d->d_name, 0) == 0;
}

static int load_cushions_handlers_plugin(const char *plugins_dir)
{
	int ret;
	int n;
	int i;
	struct dirent **namelist;
	char *path;
	void **current_plugin;

	LOGI("scanning plugins dir %s", plugins_dir);

	n = scandir(plugins_dir, &namelist, pattern_filter, alphasort);
	if (n == -1) {
		if (errno == ENOENT) {
			LOGI("no cushion handler plugin in %s, skipped",
					plugins_dir);
			return 0;
		}
		ret = -errno;
		LOGE("%s scandir(%s): %m", __func__, plugins_dir);
		return ret;
	}

	current_plugin = handler_plugins;
	for (i = 0; i < n; i++) {
		if (i > MAX_HANDLER_PLUGINS) {
			LOGW("More than %d plugins, plugin %s skipped",
					MAX_HANDLER_PLUGINS,
					namelist[i]->d_name);
			free(namelist[i]);
			continue;
		}
		ret = asprintf(&path, "%s/%s", plugins_dir,
				namelist[i]->d_name);
		if (ret == -1) {
			LOGE("%s asprintf error", __func__);
			return -ENOMEM;
		}
		LOGD("loading plugin %s", path);
		*current_plugin = dlopen(path, RTLD_NOW);
		if (!*current_plugin)
			LOGW("%s dlopen(%s): %s", __func__, path, dlerror());
		else
			current_plugin++;
		free(path);
		free(namelist[i]);
	}
	free(namelist);

	return 0;
}

int cushions_handlers_load(void)
{
	int ret;
	const char *env;

	LOGD("%s", __func__);

	/* don't register plugins twice */
	if (plugins_initialized)
		return 0;
	plugins_initialized = true;

	ret = load_cushions_handlers_plugin(CUSHIONS_DEFAULT_HANDLERS_PATH);
	if (ret < 0) {
		LOGE("load_cushions_handlers_plugin(%s): %s",
				CUSHIONS_DEFAULT_HANDLERS_PATH, strerror(-ret));
		return ret;
	}

	env = getenv(CUSHIONS_HANDLERS_ENV);
	if (env == NULL)
		return 0;

	ret = load_cushions_handlers_plugin(env);
	if (ret < 0) {
		LOGE("load_cushions_handlers_plugin(%s): %s", env,
				strerror(-ret));
		return ret;
	}

	return 0;
}

void cushions_handlers_unload(void)
{
	int ret;
	int i = MAX_HANDLER_PLUGINS;

	LOGD("%s", __func__);

	if (!plugins_initialized)
		return;
	plugins_initialized = false;

	while (i--)
		if (handler_plugins[i] != NULL) {
			ret = dlclose(handler_plugins[i]);
			if (ret != 0)
				LOGE("dlclose: %d", ret);
		}
}
