#include "SoundNodeVolume.h"
#include "ReadyOrNot.h"

void USoundNodeVolume::ParseNodes(FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances)
{
	FSoundParseParameters UpdatedParams = ParseParams;
	UpdatedParams.VolumeMultiplier *= VolumeAdjust;

	if (ChildNodes.Num() == 0)
	{
		return;
	}

	USoundNode* ChildNode = ChildNodes[0];
	const UPTRINT ChildNodeWaveInstanceHash = GetNodeWaveInstanceHash(NodeWaveInstanceHash, ChildNode, 0);
	ChildNode->ParseNodes(AudioDevice, ChildNodeWaveInstanceHash, ActiveSound, UpdatedParams, WaveInstances);
}

void USoundNodeVolume::LoadAsset(bool bAddToRoot)
{

}

void USoundNodeVolume::ClearAssetReferences()
{

}
