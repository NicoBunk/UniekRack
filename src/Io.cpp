#include "plugin.hpp"

struct Io : Module
{
	enum ParamIds
	{
		ENUMS(MUTE_PARAM, 8),
        ENUMS(GAIN_PARAMS, 8),
		ENUMS(AVG_PARAM, 2),
		ENUMS(OFFSET_PARAM, 2),
		NUM_PARAMS
	};
	enum InputIds
	{
		ENUMS(IN_INPUT, 8),
		ENUMS(SH_INPUT,2),
		NUM_INPUTS
	};
	enum OutputIds
	{
		ENUMS(MIX_OUTPUT, 2),
		NUM_OUTPUTS
	};
	enum LightIds
	{
		ENUMS(MUTE_LIGHT, 8),
		NUM_LIGHTS
	};

	bool state[8];
	dsp::BooleanTrigger muteTrigger[8];

	dsp::ClockDivider lightDivider;


	float sampledVoltage[2];

	dsp::SchmittTrigger sht[2];


	Io()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

	

    	for (int i = 0; i < 8; i++) {
           	configParam(MUTE_PARAM + i, 0.0, 1.0, 0.0, string::f("Ch %d mute", i + 1));
			configParam(GAIN_PARAMS + i, -1.f, 1.f, 0.f, string::f("Ch %d gain", i + 1), "%", 0, 100);
		}

	 	configParam(AVG_PARAM + 0, 0.0, 1.0, 0.0, "Ch 1 average mode");
		configParam(AVG_PARAM + 1, 0.0, 1.0, 0.0, "Ch 2 average mode");

		configParam(OFFSET_PARAM + 0, -10.0, 10.0, 0.0, "Ch 1 offset", " V");
		configParam(OFFSET_PARAM + 1, -10.0, 10.0, 0.0, "Ch 1 offset", " V");
			
		lightDivider.setDivision(128);

		onReset();
	}

	void onReset() override {
		for (int i = 0; i < 8; i++) {
			state[i] = true;
		}
	}
	void onRandomize() override {
		for (int i = 0; i < 8; i++) {
			state[i] = (random::uniform() < 0.5f);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		// states
		json_t* statesJ = json_array();
		for (int i = 0; i < 8; i++) {
			json_t* stateJ = json_boolean(state[i]);
			json_array_append_new(statesJ, stateJ);
		}
		json_object_set_new(rootJ, "states", statesJ);

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		// states
		json_t* statesJ = json_object_get(rootJ, "states");
		if (statesJ) {
			for (int i = 0; i < 8; i++) {
				json_t* stateJ = json_array_get(statesJ, i);
				if (stateJ)
					state[i] = json_boolean_value(stateJ);
			}
		}
	}

	void process(const ProcessArgs &args) override
	{
		float mix[2] = {};
		int count[2] = {};
		
		bool lightProcess = lightDivider.process();

		// Iterate rows for buttons
		for (int i = 0; i < 8; i++) {
	
			// Process trigger
			if (muteTrigger[i].process(params[MUTE_PARAM + i].getValue() > 0.f))
				state[i] ^= true;

			// Set light
			if(lightProcess)
				lights[MUTE_LIGHT + i].setBrightness(state[i] ? 0.9f : 0.f);
		}

		for (int i = 0; i < 2; i++) {
			// Inputs
			for (int j = 0; j < 4; j++) {


				if (state[i * 4 + j]) { 

					float gain = params[GAIN_PARAMS + 4 * i + j].getValue();
					



					mix[i] += inputs[IN_INPUT + 4 * i + j].getVoltage() * gain;
					
					if (inputs[IN_INPUT + 4 * i + j].isConnected())
						count[i]++;

				}
			}

			// Params
			if (count[i] > 0 && (int) std::round(params[AVG_PARAM + i].getValue()) == 1)
				mix[i] /= count[i];


			if (inputs[SH_INPUT + i].isConnected()) { 

				if (sht[i].process(rescale(inputs[SH_INPUT + i].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) 
					sampledVoltage[i] = mix[i] + params[OFFSET_PARAM + i].getValue();
				}
			else
				sampledVoltage[i] = mix[i] + params[OFFSET_PARAM + i].getValue();

			// Outputs
			outputs[MIX_OUTPUT + i].setVoltage(sampledVoltage[i]);
		}
    }
};

struct IoWidget : ModuleWidget
{
	IoWidget(Io *module)
	{
		setModule(module);
	
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Io.svg")));
		addChild(createWidgetCentered<ScrewSilver>(Vec(22.5, 7.5)));
		addChild(createWidgetCentered<ScrewSilver>(Vec(box.size.x - 22.5, 7.5)));
		addChild(createWidgetCentered<ScrewSilver>(Vec(22.2, 372.5)));
		addChild(createWidgetCentered<ScrewSilver>(Vec(box.size.x - 22.5, 372.5)));

    	addInput(createInputCentered<PJ301MPort>(Vec(20, 50), module, Io::IN_INPUT + 0));
		addInput(createInputCentered<PJ301MPort>(Vec(20, 90), module, Io::IN_INPUT + 1));
		addInput(createInputCentered<PJ301MPort>(Vec(20, 130), module, Io::IN_INPUT + 2));
		addInput(createInputCentered<PJ301MPort>(Vec(20, 170), module, Io::IN_INPUT + 3));
		addInput(createInputCentered<PJ301MPort>(Vec(20, 220), module, Io::IN_INPUT + 4));
		addInput(createInputCentered<PJ301MPort>(Vec(20, 260), module, Io::IN_INPUT + 5));
		addInput(createInputCentered<PJ301MPort>(Vec(20, 300), module, Io::IN_INPUT + 6));
		addInput(createInputCentered<PJ301MPort>(Vec(20, 340), module, Io::IN_INPUT + 7));

		addParam(createParamCentered<LEDBezel>(Vec(55, 50), module, Io::MUTE_PARAM + 0));
		addParam(createParamCentered<LEDBezel>(Vec(55, 90), module, Io::MUTE_PARAM + 1));
		addParam(createParamCentered<LEDBezel>(Vec(55, 130), module, Io::MUTE_PARAM + 2));
		addParam(createParamCentered<LEDBezel>(Vec(55, 170), module, Io::MUTE_PARAM + 3));
		addParam(createParamCentered<LEDBezel>(Vec(55, 220), module, Io::MUTE_PARAM + 4));
		addParam(createParamCentered<LEDBezel>(Vec(55, 260), module, Io::MUTE_PARAM + 5));
		addParam(createParamCentered<LEDBezel>(Vec(55, 300), module, Io::MUTE_PARAM + 6));
		addParam(createParamCentered<LEDBezel>(Vec(55, 340), module, Io::MUTE_PARAM + 7));

		addChild(createLightCentered<LEDBezelLight<GreenLight>>(Vec(55, 50), module, Io::MUTE_LIGHT + 0));
		addChild(createLightCentered<LEDBezelLight<GreenLight>>(Vec(55, 90), module, Io::MUTE_LIGHT + 1));
		addChild(createLightCentered<LEDBezelLight<GreenLight>>(Vec(55, 130), module, Io::MUTE_LIGHT + 2));
		addChild(createLightCentered<LEDBezelLight<GreenLight>>(Vec(55, 170), module, Io::MUTE_LIGHT + 3));
		addChild(createLightCentered<LEDBezelLight<GreenLight>>(Vec(55, 220), module, Io::MUTE_LIGHT + 4));
		addChild(createLightCentered<LEDBezelLight<GreenLight>>(Vec(55, 260), module, Io::MUTE_LIGHT + 5));
		addChild(createLightCentered<LEDBezelLight<GreenLight>>(Vec(55, 300), module, Io::MUTE_LIGHT + 6));
		addChild(createLightCentered<LEDBezelLight<GreenLight>>(Vec(55, 340), module, Io::MUTE_LIGHT + 7));
		
	    addParam(createParamCentered<RoundBlackKnob>(Vec(90, 50), module, Io::GAIN_PARAMS + 0));
		addParam(createParamCentered<RoundBlackKnob>(Vec(90, 90), module, Io::GAIN_PARAMS + 1));
		addParam(createParamCentered<RoundBlackKnob>(Vec(90, 130), module, Io::GAIN_PARAMS + 2));
		addParam(createParamCentered<RoundBlackKnob>(Vec(90, 170), module, Io::GAIN_PARAMS + 3));
		addParam(createParamCentered<RoundBlackKnob>(Vec(90, 220), module, Io::GAIN_PARAMS + 4));
		addParam(createParamCentered<RoundBlackKnob>(Vec(90, 260), module, Io::GAIN_PARAMS + 5));
		addParam(createParamCentered<RoundBlackKnob>(Vec(90, 300), module, Io::GAIN_PARAMS + 6));
		addParam(createParamCentered<RoundBlackKnob>(Vec(90, 340), module, Io::GAIN_PARAMS + 7));

		addParam(createParamCentered<CKSS>(Vec(128, 60), module, Io::AVG_PARAM + 0));
		addParam(createParamCentered<CKSS>(Vec(128, 230), module, Io::AVG_PARAM + 1));
	
	    addParam(createParamCentered<RoundBlackKnob>(Vec(128, 100), module, Io::OFFSET_PARAM + 0));
		addParam(createParamCentered<RoundBlackKnob>(Vec(128, 270), module, Io::OFFSET_PARAM + 1));

        addOutput(createOutputCentered<PJ301MPort>(Vec(128, 145), module, Io::MIX_OUTPUT + 0));
		addOutput(createOutputCentered<PJ301MPort>(Vec(128, 315), module, Io::MIX_OUTPUT + 1));
	
		addInput(createInputCentered<PJ301MPort>(Vec(128, 180), module, Io::SH_INPUT + 0));
		addInput(createInputCentered<PJ301MPort>(Vec(128, 350), module, Io::SH_INPUT + 1));
	
/*
		addInput(createInputCentered<PJ301MPort>(Vec(colGrid[0], rowGrid[3] + rowRowGrid[1] + 4), module, Io::RESET_INPUT));

		for (int r = 0; r < SEQ_NUM_ROWS; r++)
		{
			addInput(createInputCentered<PJ301MPort>(Vec(colGrid[0], rowGrid[r]), module, Io::CLOCK_INPUT + r));
			addParam(createParamCentered<RoundBlackSnapKnob>(Vec(colGrid[1], rowGrid[r]), module, Io::STEPS_PARAM + r));

			addInput(createInputCentered<PJ301MPort>(Vec(colGrid[2], rowGrid[r]), module, Io::ROW_RND_INPUT + r));
	        addInput(createInputCentered<PJ301MPort>(Vec(colGrid[2], rowGrid[r] + rowRowGrid[0]), module, Io::GATE_RND_INPUT + r * SEQ_NUM_ROW_GATES + 0));
	        addInput(createInputCentered<PJ301MPort>(Vec(colGrid[2], rowGrid[r] + rowRowGrid[1] + 4), module, Io::GATE_RND_INPUT + r * SEQ_NUM_ROW_GATES + 1));

			for (int s = 0; s < SEQ_NUM_STEPS; s++)
			{

				addParam(createParamCentered<RoundBlackKnob>(Vec(colGrid[3] + s * stepWidth, rowGrid[r]), module, Io::ROW_PARAM + r * SEQ_NUM_STEPS + s));

				addParam(createParamCentered<LEDButton>(Vec(colGrid[3] + s * stepWidth, rowGrid[r] + rowRowGrid[0]), module, Io::GATE_PARAM + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 0));

				addChild(createLightCentered<MediumLight<GreenLight>>(Vec(colGrid[3] + s * stepWidth, rowGrid[r] + rowRowGrid[0]), module, Io::GATE_LIGHTS + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 0));

				addParam(createParamCentered<LEDButton>(Vec(colGrid[3] + s * stepWidth, rowGrid[r] + rowRowGrid[1]), module, Io::GATE_PARAM + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 1));

				addChild(createLightCentered<MediumLight<GreenLight>>(Vec(colGrid[3] + s * stepWidth, rowGrid[r] + rowRowGrid[1]), module, Io::GATE_LIGHTS + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 1));
			};

			addOutput(createOutputCentered<PJ301MPort>(Vec(colGrid[4], rowGrid[r]), module, Io::ROW_OUTPUT + r));

			addOutput(createOutputCentered<PJ301MPort>(Vec(colGrid[4], rowGrid[r] + rowRowGrid[0]), module, Io::GATE_OUTPUT + r * SEQ_NUM_ROW_GATES + 0));
			addOutput(createOutputCentered<PJ301MPort>(Vec(colGrid[4], rowGrid[r] + rowRowGrid[1] + 4), module, Io::GATE_OUTPUT + r * SEQ_NUM_ROW_GATES + 1));
		};
    
    */
	};
   
};

Model *modelIo = createModel<Io, IoWidget>("Io");