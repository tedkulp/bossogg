#include "gstsong.h"

void gstreamer_init() {

	gst_init(NULL, NULL);
}

int start(song *thesong) {

	printf("beginning of start\n");
        int result = OK;

        if (thesong->initdone) {
                finish(thesong);
        }

	printf("before pipeline\n");
        //Setup the pipeline
        thesong->pipeline = gst_pipeline_new("pipeline");

	printf("before filesrc\n");
        //Setup the file source -- needs to have more logic if http or other streaming protocol
        if (!(thesong->filesrc = gst_element_factory_make("gnomevfssrc", "source"))) {
                if (!(thesong->filesrc = gst_element_factory_make("filesrc", "source"))) {
                        //std::cerr << "Can't create source...exiting!" << std::endl;
                        //exit(1);
                }
        }
        g_object_set(G_OBJECT(thesong->filesrc), "location", thesong->filename, NULL);

        //Setup the decoder
	printf("before decoder\n");
        thesong->decoder = gst_element_factory_make("spider", "decoder");

	printf("before audio device\n");
        //Open the audio device.  Logic needs to go in for a user selected sink
        if (!(thesong->audiosink = gst_element_factory_make (thesong->sink, "play_audio"))) {
                if (!(thesong->audiosink = gst_element_factory_make ("alsasink", "play_audio"))) {
                        if (!(thesong->audiosink = gst_element_factory_make ("osssink", "play_audio"))) {
				if (!(thesong->audiosink = gst_element_factory_make ("fakesink", "play_audio"))) {
					//std::cerr << "Can't open audio...exiting!" << std::endl;
					//exit(-1);
				}
                        }
                }
        }

        //gst_bin_add_many(GST_BIN(thesong->pipeline), thesong->filesrc, thesong->decoder, thesong->audiosink, NULL);
	printf("before adds\n");
	gst_bin_add(GST_BIN(thesong->pipeline), thesong->filesrc);
	gst_bin_add(GST_BIN(thesong->pipeline), thesong->decoder);
	gst_bin_add(GST_BIN(thesong->pipeline), thesong->audiosink);
	printf("after adds\n");
        if (!gst_element_link_many(thesong->filesrc, thesong->decoder, thesong->audiosink, NULL)) {
                result = SWITCH_SONGS;
                thesong->initdone = FALSE;
        }
        else {
                gst_element_set_state(thesong->pipeline, GST_STATE_PLAYING);
                thesong->initdone = TRUE;
        }

        return result;
}

int play(song *thesong) {

        int result = OK;

        if (!thesong->initdone) {
                start(thesong);
        }
        else {
                if (!gst_bin_iterate(GST_BIN(thesong->pipeline))) {
                        result = SWITCH_SONGS;
                }
                else {
                        result = OK;
                        GstPad *srcpad;
                        srcpad = gst_element_get_pad(thesong->decoder, "src_0");
                        gint64 blah;
                        GstFormat format = GST_FORMAT_TIME;
                        gst_pad_query(srcpad, GST_QUERY_POSITION, &format, &blah);
                        thesong->playedseconds = (double)blah/GST_SECOND;
                }
        }

        return result;
}

int finish(song *thesong) {

        int result = SWITCH_SONGS;

        if (thesong->initdone) {
                //close it down
                //log("running set_state");
                gst_element_set_state(thesong->pipeline, GST_STATE_NULL);
                //log("running unref");
                gst_object_unref(GST_OBJECT(thesong->pipeline));
                //log("finished finishing");
                thesong->initdone = FALSE;
        }

        return result;
}

int restart(song *thesong) {

        int result = OK;

        if (thesong->initdone) {
                GstPad *srcpad;
                gst_element_set_state(thesong->pipeline, GST_STATE_PAUSED);
                srcpad = gst_element_get_pad(thesong->decoder, "src_0");
                result = gst_pad_send_event(srcpad, gst_event_new_seek((GstSeekType)GST_FORMAT_TIME, (gint64)0));
                gst_element_set_state(thesong->pipeline, GST_STATE_PLAYING);
        }
        else {
                result = ERROR;
        }

        return result;
}

int seek(song *thesong, int amt) {

        int result = OK;

        if (thesong->initdone) {
                GstPad *srcpad;
                gst_element_set_state(thesong->pipeline, GST_STATE_PAUSED);
                srcpad = gst_element_get_pad(thesong->decoder, "src_0");
		result = gst_pad_send_event(srcpad, gst_event_new_seek((GstSeekType)GST_FORMAT_TIME, (gint64)(amt * GST_SECOND) + ((int)thesong->playedseconds * GST_SECOND)));
                gst_element_set_state(thesong->pipeline, GST_STATE_PLAYING);
        }
        else {
                result = ERROR;
        }

        return result;
}
