#include<obs-module.h>

struct webrtc_sportsbooth {
    char *token;
};

static const char *webrtc_sportsbooth_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WebRTC Sportsbooth Streaming Server");
}

static void webrtc_sportsbooth_update(void *data, obs_data_t *settings)
{
	struct webrtc_sportsbooth *service = data;

	bfree(service->token);

	service->token = bstrdup(obs_data_get_string(settings, "token"));
}

static void webrtc_sportsbooth_destroy(void *data)
{
	struct webrtc_sportsbooth *service = data;

	bfree(service->token);
	bfree(service);
}

static void *webrtc_sportsbooth_create(obs_data_t *settings, obs_service_t *service)
{
	struct webrtc_sportsbooth *data = bzalloc(sizeof(struct webrtc_sportsbooth));
	webrtc_sportsbooth_update(data, settings);

	UNUSED_PARAMETER(service);
	return data;
}

static obs_properties_t *webrtc_sportsbooth_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_text(ppts, "token", obs_module_text("Token"),OBS_TEXT_PASSWORD);


	return ppts;
}

static const char *webrtc_sportsbooth_url(void *data)
{
	return "wss://worsh.tv/websocket/websocket";
}


static const char *webrtc_sportsbooth_room(void *data)
{
	return "1";
}

static const char *webrtc_sportsbooth_token(void *data)
{
	struct webrtc_sportsbooth *service = data;
	return service->token;
}

struct obs_service_info webrtc_sportsbooth_service = {
	.id             = "webrtc_sportsbooth",
	.get_name       = webrtc_sportsbooth_name,
	.create         = webrtc_sportsbooth_create,
	.destroy        = webrtc_sportsbooth_destroy,
	.update         = webrtc_sportsbooth_update,
	.get_properties = webrtc_sportsbooth_properties,
	.get_url        = webrtc_sportsbooth_url,
	.get_room       = webrtc_sportsbooth_room,
	.get_password   = webrtc_sportsbooth_token
};