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
		const MidiMessageSequence* msgSeq = midiFile.getTrack(i);

		OwnedArray<PluginDescription> results;
		String plugFile = "C:\\VST\\FMMF.dll";
		VSTPluginFormat vstFormat;
		vstFormat.findAllTypesForFile(results, plugFile);
		if (results.size() > 0) {
			std::cout << "Found " << results.size() << " plugin(s) matching file " << plugFile << std::endl;

			int secsToRender = 10;
			double sampleRate = 44100;
			int totalSizeInSamples = static_cast<int>(44100 * secsToRender);
			AudioPluginInstance* plugInst = vstFormat.createInstanceFromDescription(*results[0], sampleRate, totalSizeInSamples);
			if (!plugInst) {
				cout << "Failed to load plugin " << plugFile << endl;
				continue;
			}

			int numInputChannels = plugInst->getTotalNumInputChannels();
			int numOutputChannels = plugInst->getTotalNumOutputChannels();
			cout << "Plugin input channels: " << numInputChannels << " output channels: " << numOutputChannels
				<< " Current program: " << plugInst->getCurrentProgram() << endl;

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

			plugInst->prepareToPlay(sampleRate, totalSizeInSamples);
			plugInst->processBlock(buffer, midiMessages);

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
			delete plugInst;
		}
		else {
			cerr << "Could not find plugin from file " << plugFile << endl;
		}
	}

    return 0;
}