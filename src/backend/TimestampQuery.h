#pragma once

#include "LingzeVK.h"

namespace lz
{
	struct TimestampQuery
	{
		TimestampQuery(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, uint32_t maxTimestampCount);

		void ResetQueryPool(vk::CommandBuffer commandBuffer);

		void AddTimestamp(vk::CommandBuffer commandBuffer, size_t timestampName,
		                  vk::PipelineStageFlagBits pipelineStage);

		struct QueryResult
		{
			struct TimestampData
			{
				size_t timestampName;
				double time;
			};

			const TimestampData* data;
			size_t size;
		};

		QueryResult QueryResults(vk::Device logicalDevice);

	private:
		std::vector<uint64_t> queryResults;
		std::vector<QueryResult::TimestampData> timestampDatas;
		vk::UniqueQueryPool queryPool;
		uint32_t currTimestampIndex;
		float timestampPeriod;
		uint64_t timestampMask;
	};
}
