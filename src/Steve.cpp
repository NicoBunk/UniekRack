#include "plugin.hpp"
#include "UniekComponents.hpp"

#define SEQ_NUM_ROWS 4
#define SEQ_NUM_STEPS 16
#define SEQ_NUM_ROW_GATES 2

struct Steve : Module
{
	enum ParamIds
	{
		ENUMS(STEPS_PARAM, SEQ_NUM_ROWS),
		ENUMS(MODE_PARAM, SEQ_NUM_ROWS),
		ENUMS(ROW_PARAM, SEQ_NUM_ROWS *SEQ_NUM_STEPS),
		ENUMS(GATE_PARAM, SEQ_NUM_ROWS *SEQ_NUM_STEPS *SEQ_NUM_ROW_GATES),
		NUM_PARAMS
	};
	enum InputIds
	{
		RESET_INPUT,
		ENUMS(CLOCK_INPUT, SEQ_NUM_ROWS),
		ENUMS(ROW_RND_INPUT, SEQ_NUM_ROWS),
		ENUMS(GATE_RND_INPUT, SEQ_NUM_ROWS *SEQ_NUM_ROW_GATES),
		NUM_INPUTS
	};
	enum OutputIds
	{
		ENUMS(GATE_OUTPUT, SEQ_NUM_ROWS *SEQ_NUM_ROW_GATES),
		ENUMS(ROW_OUTPUT, SEQ_NUM_ROWS),
		NUM_OUTPUTS
	};
	enum LightIds
	{
		ENUMS(GATE_LIGHTS, SEQ_NUM_ROWS *SEQ_NUM_STEPS *SEQ_NUM_ROW_GATES),
		NUM_LIGHTS
	};

	dsp::SchmittTrigger clockTriggers[SEQ_NUM_ROWS];
	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger gateTriggers[SEQ_NUM_ROWS * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES];
	dsp::SchmittTrigger rowRndTriggers[SEQ_NUM_ROWS];
	dsp::SchmittTrigger gateRndTriggers[SEQ_NUM_ROWS * SEQ_NUM_ROW_GATES];

	int index[SEQ_NUM_ROWS] = {};
	bool gates[SEQ_NUM_ROWS * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES] = {};

	dsp::ClockDivider lightDivider;



	Steve()
	{
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		for (int r = 0; r < SEQ_NUM_ROWS; r++)
		{
			configParam(STEPS_PARAM + r, 1.f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS));


			configParam(MODE_PARAM + r, 0.0, 3.0, 0.0, "Mode", "");

			for (int s = 0; s < SEQ_NUM_STEPS; s++)
			{
				configParam(ROW_PARAM + r * SEQ_NUM_STEPS + s, 0.f, 10.f, 0.f);
				configParam(GATE_PARAM + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 0, 0.f, 1.f, 0.f);
				configParam(GATE_PARAM + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 1, 0.f, 1.f, 0.f);
			}
		}

		lightDivider.setDivision(128);

		onReset();
	}

	void onReset() override
	{
		for (int i = 0; i < SEQ_NUM_ROWS * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES; i++)
		{
			gates[i] = true;
		}
		for (int i = 0; i < SEQ_NUM_ROWS; i++)
		{
			index[i] = 0;
		}
	}
 
	void onRandomize() override
	{
		for (int i = 0; i < SEQ_NUM_ROWS * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES; i++)
		{
			gates[i] = (random::uniform() > 0.5f);
		}



        

	}

	 
	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		// gates
		json_t* gatesJ = json_array();
		for (int i = 0; i < SEQ_NUM_ROWS * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES; i++) {
			json_array_insert_new(gatesJ, i, json_integer((int) gates[i]));
		}
		json_object_set_new(rootJ, "gates", gatesJ);

		return rootJ;
	}
	
	void dataFromJson(json_t* rootJ) override {
		// gates
		json_t* gatesJ = json_object_get(rootJ, "gates");
		if (gatesJ) {
			for (int i = 0; i < SEQ_NUM_ROWS * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES; i++) {
				json_t* gateJ = json_array_get(gatesJ, i);
				if (gateJ)
					gates[i] = !!json_integer_value(gateJ);
			}
		}
	}


	void setIndex(int row, int index)
	{
		int numSteps = (int)clamp(std::round(params[STEPS_PARAM + row].getValue() /* + inputs[STEPS_INPUT].getVoltage() */), 1.f, (float)(SEQ_NUM_STEPS));

		this->index[row] = index;
		if (this->index[row] >= numSteps)
			this->index[row] = 0;
	}

	void process(const ProcessArgs &args) override
	{
		bool gateIn[SEQ_NUM_ROWS] = {false, false, false, false};
		bool trigIn[SEQ_NUM_ROWS] = {false, false, false, false};

		for (int r = 0; r < SEQ_NUM_ROWS; r++)
		{
			if (inputs[ROW_RND_INPUT + r].isConnected()) { 
				if (rowRndTriggers[r].process(inputs[ROW_RND_INPUT + r].getVoltage())) {
					for (int s = 0; s < SEQ_NUM_STEPS; s++)
					{
						params[ROW_PARAM + r * SEQ_NUM_STEPS + s   ].setValue(random::uniform() * 10);
					}
				}
			}

			for (int g = 0; g < SEQ_NUM_ROW_GATES; g++){
				if (inputs[GATE_RND_INPUT + r * SEQ_NUM_ROW_GATES + g].isConnected()) { 
					if (gateRndTriggers[r * SEQ_NUM_ROW_GATES + g].process(inputs[GATE_RND_INPUT + r * SEQ_NUM_ROW_GATES + g].getVoltage())) {
						for (int s = 0; s < SEQ_NUM_STEPS; s++)
						{
							gates[r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + g] = (random::uniform() > 0.5f);
						}
					}
				}
			}





			if (inputs[CLOCK_INPUT + r].isConnected())
			{
				trigIn[r] = clockTriggers[r].process(inputs[CLOCK_INPUT + r].getVoltage());
				gateIn[r] = clockTriggers[r].isHigh();
			}
			else if (r > 0)
			{
				trigIn[r] = trigIn[r - 1];
				gateIn[r] = gateIn[r - 1];
			}

			if (trigIn[r])
				setIndex(r, index[r] + 1);
		}

		// Reset
		if (resetTrigger.process(inputs[RESET_INPUT].getVoltage()))
		{
			for (int r = 0; r < SEQ_NUM_ROWS; r++)
				setIndex(r, 0);
		}

		// Gate buttons
		int i = 0;
		bool lightProcess = lightDivider.process();

		for (int r = 0; r < SEQ_NUM_ROWS; r++)
		{
			for (int s = 0; s < SEQ_NUM_STEPS; s++)
			{
				for (int g = 0; g < SEQ_NUM_ROW_GATES; g++)
				{
					if (gateTriggers[i].process(params[GATE_PARAM + i].getValue()))
						gates[i] = !gates[i];

					// std::cout << "gate " << i << " " << gateIn1 << " " << s << " " << index[r] << " " << gates[i] << " " << r * SEQ_NUM_ROW_GATES + g << " " << ( gateIn1 && s == index[r] && gates[i]) << std::endl;

					if (lightProcess)
						lights[GATE_LIGHTS + i].setBrightness((gateIn[r] && s == index[r]) ? (gates[i] ? 1.f : 0.33) : (gates[i] ? 0.5 : 0.0));

					i++; // = r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + g;
				}
			}

			for (int g = 0; g < SEQ_NUM_ROW_GATES; g++)
				outputs[GATE_OUTPUT + r * SEQ_NUM_ROW_GATES + g].setVoltage((gateIn[r] && gates[r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + index[r] * SEQ_NUM_ROW_GATES + g]) ? 10.f : 0.f);

			outputs[ROW_OUTPUT + r].setVoltage(params[ROW_PARAM + r * SEQ_NUM_STEPS + index[r]].getValue());
		}
	};
};

struct SteveWidget : ModuleWidget
{
	SteveWidget(Steve *module)
	{
		setModule(module);
		// Panel
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Steve.svg")));
		addChild(createWidgetCentered<ScrewSilver>(Vec(37.5, 7.5)));
		addChild(createWidgetCentered<ScrewSilver>(Vec(box.size.x - 37.5, 7.5)));
		addChild(createWidgetCentered<ScrewSilver>(Vec(37.5, 372.5)));
		addChild(createWidgetCentered<ScrewSilver>(Vec(box.size.x - 37.5, 372.5)));

		static const int stepWidth = 36;
		static const int rowGrid[4] = {50, 135, 220, 305};
		static const int rowRowGrid[2] = {26, 48};
		static const int colGrid[5] = {20, 52, 84, 116, 688};

		addInput(createInputCentered<PJ301MPort>(Vec(colGrid[0], rowGrid[3] + rowRowGrid[1] + 4), module, Steve::RESET_INPUT));

		for (int r = 0; r < SEQ_NUM_ROWS; r++)
		{
			addInput(createInputCentered<PJ301MPort>(Vec(colGrid[0], rowGrid[r]), module, Steve::CLOCK_INPUT + r));
			addParam(createParamCentered<RoundBlackSnapKnob>(Vec(colGrid[1], rowGrid[r]), module, Steve::STEPS_PARAM + r));

			addParam(createParam<CKSSFour>(Vec(colGrid[1] - 13, rowGrid[r] + 21), module, Steve::MODE_PARAM + r));


			addInput(createInputCentered<PJ301MPort>(Vec(colGrid[2], rowGrid[r]), module, Steve::ROW_RND_INPUT + r));
	        addInput(createInputCentered<PJ301MPort>(Vec(colGrid[2], rowGrid[r] + rowRowGrid[0]), module, Steve::GATE_RND_INPUT + r * SEQ_NUM_ROW_GATES + 0));
	        addInput(createInputCentered<PJ301MPort>(Vec(colGrid[2], rowGrid[r] + rowRowGrid[1] + 4), module, Steve::GATE_RND_INPUT + r * SEQ_NUM_ROW_GATES + 1));

			for (int s = 0; s < SEQ_NUM_STEPS; s++)
			{

				addParam(createParamCentered<RoundBlackKnob>(Vec(colGrid[3] + s * stepWidth, rowGrid[r]), module, Steve::ROW_PARAM + r * SEQ_NUM_STEPS + s));

				addParam(createParamCentered<LEDButton>(Vec(colGrid[3] + s * stepWidth, rowGrid[r] + rowRowGrid[0]), module, Steve::GATE_PARAM + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 0));
				addChild(createLightCentered<MediumLight<GreenLight>>(Vec(colGrid[3] + s * stepWidth, rowGrid[r] + rowRowGrid[0]), module, Steve::GATE_LIGHTS + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 0));

				addParam(createParamCentered<LEDButton>(Vec(colGrid[3] + s * stepWidth, rowGrid[r] + rowRowGrid[1]), module, Steve::GATE_PARAM + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 1));
				addChild(createLightCentered<MediumLight<GreenLight>>(Vec(colGrid[3] + s * stepWidth, rowGrid[r] + rowRowGrid[1]), module, Steve::GATE_LIGHTS + r * SEQ_NUM_STEPS * SEQ_NUM_ROW_GATES + s * SEQ_NUM_ROW_GATES + 1));
			};

			addOutput(createOutputCentered<PJ301MPort>(Vec(colGrid[4], rowGrid[r]), module, Steve::ROW_OUTPUT + r));

			addOutput(createOutputCentered<PJ301MPort>(Vec(colGrid[4], rowGrid[r] + rowRowGrid[0]), module, Steve::GATE_OUTPUT + r * SEQ_NUM_ROW_GATES + 0));
			addOutput(createOutputCentered<PJ301MPort>(Vec(colGrid[4], rowGrid[r] + rowRowGrid[1] + 4), module, Steve::GATE_OUTPUT + r * SEQ_NUM_ROW_GATES + 1));
		};
	};
};

Model *modelSteve = createModel<Steve, SteveWidget>("Steve");