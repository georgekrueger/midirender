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

class MyAudioPlayHead : public juce::AudioPlayHead
{
public:
	virtual bool getCurrentPosition(CurrentPositionInfo& result)
	{
		result = posInfo;
		return true;
	}

	CurrentPositionInfo posInfo;
};

MyAudioPlayHead playHead;

//==============================================================================
int main (int argc, char* argv[])
{
	if (argc != 3) {
		cout << "Usage: <prog> <midi input file> <wav output file>" << endl;
		return 0;
	}
	File inMidiFile = File(argv[1]);
	File outWavFile = File(argv[2]);

	//File inMidiFile = File("C:\\Users\\GeorgeKrueger\\Documents\\GitHub\\pymusic\\out.mid");
	//File outWavFile = File("C:\\Users\\GeorgeKrueger\\Documents\\GitHub\\pymusic\\out.wav");

	FileInputStream fileStream(inMidiFile);
	juce::MidiFile midiFile;
	midiFile.readFrom(fileStream);
	int numTracks = midiFile.getNumTracks();
	midiFile.convertTimestampTicksToSeconds();
	std::cout << "Opened midi file: " << inMidiFile.getFileName() << " Tracks: " << numTracks << std::endl;;

	playHead.posInfo.bpm = 120;
	playHead.posInfo.isPlaying = true;
	playHead.posInfo.timeInSamples = 0;
	playHead.posInfo.timeInSeconds = 0;
	playHead.posInfo.timeSigNumerator = 4;
	playHead.posInfo.timeSigDenominator = 4;

	for (int i = 0; i < numTracks; ++i)
	{
		const juce::MidiMessageSequence* msgSeq = midiFile.getTrack(i);

		double trackLengthSeconds = 0;
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
			if (midiMsg.isMetaEvent() && midiMsg.isEndOfTrackMetaEvent()) {
				//int oetDataLength = midiMsg.getMetaEventLength();
				//const uint8* oetData = midiMsg.getMetaEventData();
				//std::cout << "Found end of track event data size: " << oetDataLength << " data: " << oetData << std::endl;
				trackLengthSeconds = midiMsg.getTimeStamp();
				std::cout << "Track length in seconds: " << trackLengthSeconds << std::endl;
			}
		}

		if (trackLengthSeconds == 0) {
			std::cerr << "Skipping track " << i << " since it has zero length" << std::endl;
			continue;
		}

		if (plugFile.isEmpty()) {
			plugFile = "C:\\VST\\helm.dll";
			std::cout << "No plug found for track. Defaulting to: " << plugFile << std::endl;
			//std::cerr << "Skipping track " << i << ". No instrument found." << std::endl;
			//continue;
		}
		else {
			std::cout << "Found plugin file '" << plugFile << "' from track " << i << std::endl;
		}

		OwnedArray<PluginDescription> results;
		VSTPluginFormat vstFormat;
		vstFormat.findAllTypesForFile(results, plugFile);
		if (results.size() > 0) {
			std::cout << "Found " << results.size() << " plugin(s) in file '" << plugFile << "'" << std::endl;

			int blockSize = 1024;
			double sampleRate = 44100;
			int totalSizeInSamples = ((static_cast<int>(44100 * trackLengthSeconds) / 1024) + 1) * 1024;
			cout << "Total samples to render " << totalSizeInSamples << endl;
			juce::AudioPluginInstance* plugInst = vstFormat.createInstanceFromDescription(*results[0], sampleRate, blockSize);
			if (!plugInst) {
				cout << "Failed to load plugin " << plugFile << endl;
				continue;
			}

			AudioProcessorGraph* graph = new AudioProcessorGraph();
			graph->setPlayConfigDetails(0, 2, sampleRate, blockSize);
			graph->setPlayHead(&playHead);
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
			graph->addConnection(MIDI_IN_ID, AudioProcessorGraph::midiChannelIndex, 1000, AudioProcessorGraph::midiChannelIndex);
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

			/*int numParams = plugInst->getNumParameters();
			cout << "Num Parameters: " << numParams << endl;
			for (int p = 0; p < numParams; ++p)
			{
				std::cout << "Param " << p << ": " << plugInst->getParameterName(p);
				if (!plugInst->getParameterLabel(p).isEmpty()) {
					cout << "(" << plugInst->getParameterLabel(p) << ")";
				}
				cout << " = " << plugInst->getParameter(p) << endl;
			}*/

			int maxChannels = std::max(numInputChannels, numOutputChannels);
			AudioBuffer<float> entireAudioBuffer(maxChannels, totalSizeInSamples);
			entireAudioBuffer.clear();
			unsigned int midiSeqPos = 0;

			graph->releaseResources();
			graph->prepareToPlay(sampleRate, blockSize);

			cout << "Num midi events: " << msgSeq->getNumEvents() << endl;

			// Render the audio in blocks
			for (int t = 0; t < totalSizeInSamples; t += blockSize)
			{
				//cout << "processing block " << t << " to " << t + blockSize << endl;
				MidiBuffer midiBuffer;
				for (int j = midiSeqPos; j < msgSeq->getNumEvents(); ++j)
				{
					MidiMessageSequence::MidiEventHolder* midiEventHolder = msgSeq->getEventPointer(j);
					MidiMessage midiMsg = midiEventHolder->message;
					int samplePos = static_cast<int>(midiMsg.getTimeStamp() * sampleRate);
					if (samplePos >= t && samplePos < t + blockSize) {
						if (midiMsg.isNoteOnOrOff()) {
							if (midiMsg.isNoteOn()) {
								cout << "note on event (" << midiMsg.getNoteNumber() << ") at " << samplePos << "(" << midiMsg.getTimeStamp() << "s) bufferpos=" << (samplePos - t) << endl;
							}
							else if (midiMsg.isNoteOff()) {
								cout << "note off event (" << midiMsg.getNoteNumber() << ") at " << samplePos << "(" << midiMsg.getTimeStamp() << "s) bufferpos=" << (samplePos - t) << endl;
							}
							midiBuffer.addEvent(midiMsg, samplePos - t);
						}
						else if (midiMsg.isProgramChange()) {
							program = midiMsg.getProgramChangeNumber();
							plugInst->setCurrentProgram(program);
						}
						midiSeqPos++;
					}
					else {
						break;
					}
				}

				playHead.posInfo.timeInSamples = t;
				playHead.posInfo.timeInSeconds = t / sampleRate;

				AudioBuffer<float> blockAudioBuffer(entireAudioBuffer.getNumChannels(), blockSize);
				blockAudioBuffer.clear();
				graph->processBlock(blockAudioBuffer, midiBuffer);

				for (int ch = 0; ch < entireAudioBuffer.getNumChannels(); ++ch) {
					entireAudioBuffer.addFrom(ch, t, blockAudioBuffer, ch, 0, blockSize);
				}
			}

			if (outWavFile.exists()) {
				outWavFile.deleteFile();
			}
			FileOutputStream* fileOutputStream = outWavFile.createOutputStream();
			WavAudioFormat wavFormat;
			StringPairArray metadataValues;
			juce::AudioFormatWriter* wavFormatWriter = wavFormat.createWriterFor(
				fileOutputStream, sampleRate, 2, 16, metadataValues, 0);
			bool writeAudioDataRet = wavFormatWriter->writeFromAudioSampleBuffer(entireAudioBuffer, 0, entireAudioBuffer.getNumSamples());
			wavFormatWriter->flush();

			cout << "Done writing to output file " << outWavFile.getFileName() << " . Write return value: "
				<< (int)writeAudioDataRet << endl;

			delete wavFormatWriter;
		}
		else {
			cerr << "Could not find plugin from file " << plugFile << endl;
		}
	}

    return 0;
}
