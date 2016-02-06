/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;
using juce::AudioProcessorGraph;


//==============================================================================
int main (int argc, char* argv[])
{
	if (argc != 3) {
		cout << "Usage: <prog> <midi input file> <wav output file>" << endl;
		return 0;
	}

	File inMidiFile = File(argv[1]);
	File outWavFile = File(argv[2]);

	FileInputStream fileStream(inMidiFile);
	juce::MidiFile midiFile;
	midiFile.readFrom(fileStream);
	int numTracks = midiFile.getNumTracks();
	midiFile.convertTimestampTicksToSeconds();
	std::cout << "Opened midi file: " << inMidiFile.getFileName() << " Tracks: " << numTracks << std::endl;;

	for (int i = 0; i < numTracks; ++i)
	{
		const juce::MidiMessageSequence* msgSeq = midiFile.getTrack(i);

		String plugFile = "";
		int program = 0;
		for (int j = 0; j < msgSeq->getNumEvents(); ++j)
		{
			juce::MidiMessageSequence::MidiEventHolder* midiEventHolder = msgSeq->getEventPointer(j);
			juce::MidiMessage midiMsg = midiEventHolder->message;
			if (midiMsg.isMetaEvent() && midiMsg.getMetaEventType() == 0x04) {
				// Instrument meta event
				int instrLength = midiMsg.getMetaEventLength();
				const juce::uint8* instrChars = midiMsg.getMetaEventData();
				String instrName((char*)instrChars, instrLength);
				plugFile = instrName;
			}
			else if (midiMsg.isProgramChange()) {
				program = midiMsg.getProgramChangeNumber();
			}
		}

		if (plugFile.isEmpty()) {
			std::cerr << "Skipping track " << i << ". No instrument found." << std::endl;
			continue;
		}
		else {
			std::cout << "Found plugin file '" << plugFile << "' from track " << i << std::endl;
		}

		OwnedArray<PluginDescription> results;
		VSTPluginFormat vstFormat;
		vstFormat.findAllTypesForFile(results, plugFile);
		if (results.size() > 0) {
			std::cout << "Found " << results.size() << " plugin(s) in file '" << plugFile << "'" << std::endl;

			int secsToRender = 10;
			double sampleRate = 44100;
			int totalSizeInSamples = static_cast<int>(44100 * secsToRender);
			juce::AudioPluginInstance* plugInst = vstFormat.createInstanceFromDescription(*results[0], sampleRate, totalSizeInSamples);
			if (!plugInst) {
				cout << "Failed to load plugin " << plugFile << endl;
				continue;
			}

			AudioProcessorGraph* graph = new AudioProcessorGraph();
			graph->setPlayConfigDetails(0, 2, sampleRate, totalSizeInSamples);
			graph->addNode(plugInst, 1000);

			int AUDIO_IN_ID = 101;
			int AUDIO_OUT_ID = 102;
			int MIDI_IN_ID = 103;
			juce::AudioPluginInstance* audioInNode = new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
			juce::AudioPluginInstance* audioOutNode = new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
			juce::AudioPluginInstance* midiInNode = new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
			graph->addNode(audioInNode, AUDIO_IN_ID);
			graph->addNode(audioOutNode, AUDIO_OUT_ID);
			graph->addNode(midiInNode, MIDI_IN_ID);

			graph->addConnection(AUDIO_IN_ID, 0, 1000, 0);
			graph->addConnection(AUDIO_IN_ID, 1, 1000, 1);
			graph->addConnection(MIDI_IN_ID, 0, 1000, 0);
			graph->addConnection(1000, 0, AUDIO_OUT_ID, 0);
			graph->addConnection(1000, 1, AUDIO_OUT_ID, 1);

			plugInst->setCurrentProgram(program);

			int numInputChannels = plugInst->getTotalNumInputChannels();
			int numOutputChannels = plugInst->getTotalNumOutputChannels();
			cout << "----- Plugin Information -----" << endl;
			cout << "Input channels : " << numInputChannels << endl;
			cout << "Output channels : " << numOutputChannels << endl;
			cout << "Num Programs: " << plugInst->getNumPrograms() << endl;
			cout << "Current program: " << plugInst->getCurrentProgram() << endl;

			int maxChannels = std::max(numInputChannels, numOutputChannels);
			AudioBuffer<float> buffer(maxChannels, totalSizeInSamples);

			MidiBuffer midiMessages;
			for (int j = 0; j < msgSeq->getNumEvents(); ++j)
			{
				MidiMessageSequence::MidiEventHolder* midiEventHolder = msgSeq->getEventPointer(j);
				MidiMessage midiMsg = midiEventHolder->message;
				int samplePos = static_cast<int>(midiMsg.getTimeStamp() * sampleRate);
				midiMessages.addEvent(midiMsg, samplePos);
			}

			graph->releaseResources();
			graph->prepareToPlay(sampleRate, totalSizeInSamples);

			int numParams = plugInst->getNumParameters();
			cout << "Num Parameters: " << numParams << endl;
			for (int p = 0; p < numParams; ++p)
			{
				std::cout << "Param " << p << ": " << plugInst->getParameterName(p);
				if (!plugInst->getParameterLabel(p).isEmpty()) {
					cout << "(" << plugInst->getParameterLabel(p) << ")";
				}
				cout << " = " << plugInst->getParameter(p) << endl;
			}

			graph->processBlock(buffer, midiMessages);

			if (outWavFile.exists()) {
				outWavFile.deleteFile();
			}
			FileOutputStream* fileOutputStream = outWavFile.createOutputStream();
			WavAudioFormat wavFormat;
			StringPairArray metadataValues;
			juce::AudioFormatWriter* wavFormatWriter = wavFormat.createWriterFor(
				fileOutputStream, sampleRate, 2, 16, metadataValues, 0);
			bool writeAudioDataRet = wavFormatWriter->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
			wavFormatWriter->flush();

			cout << "Done writing to output file " << outWavFile.getFileName() << " . Write return value: "
				<< (int)writeAudioDataRet << endl;

			delete wavFormatWriter;
			//delete plugInst;
		}
		else {
			cerr << "Could not find plugin from file " << plugFile << endl;
		}
	}

    return 0;
}
