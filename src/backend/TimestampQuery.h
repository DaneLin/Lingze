#pragma once
namespace lz
{
	struct TimestampQuery
	{
		TimestampQuery(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, uint32_t maxTimestampCount)
		{
			auto queryPoolInfo = vk::QueryPoolCreateInfo()
			                     .setQueryType(vk::QueryType::eTimestamp)
			                     .setQueryCount(maxTimestampCount);
			this->queryPool = logicalDevice.createQueryPoolUnique(queryPoolInfo);
			this->timestampDatas.resize(maxTimestampCount);
			this->queryResults.resize(maxTimestampCount);
			this->currTimestampIndex = 0;
			this->timestampPeriod = physicalDevice.getProperties().limits.timestampPeriod;
		}

		void ResetQueryPool(vk::CommandBuffer commandBuffer)
		{
			commandBuffer.resetQueryPool(queryPool.get(), 0, static_cast<uint32_t>(timestampDatas.size()));
			currTimestampIndex = 0;
		}

		void AddTimestamp(vk::CommandBuffer commandBuffer, size_t timestampName,
		                  vk::PipelineStageFlagBits pipelineStage)
		{
			assert(currTimestampIndex < timestampDatas.size());
			commandBuffer.writeTimestamp(pipelineStage, queryPool.get(), currTimestampIndex);
			timestampDatas[currTimestampIndex].timestampName = timestampName;
			currTimestampIndex++;
		}

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

		QueryResult QueryResults(vk::Device logicalDevice)
		{
			std::fill(queryResults.begin(), queryResults.end(), 0);
			auto queryRes = logicalDevice.getQueryPoolResults(queryPool.get(), 0, currTimestampIndex,
			                                                  queryResults.size() * sizeof(std::uint64_t),
			                                                  queryResults.data(), sizeof(std::uint64_t),
			                                                  vk::QueryResultFlagBits::e64 |
			                                                  vk::QueryResultFlagBits::eWait);
			//assert(queryRes == vk::Result::eSuccess);
			if (queryRes == vk::Result::eSuccess)
			{
				for (uint32_t timestampIndex = 0; timestampIndex < currTimestampIndex; timestampIndex++)
				{
					timestampDatas[timestampIndex].time = (queryResults[timestampIndex] - queryResults[0]) * double(
						timestampPeriod / 1e9); //in seconds
				}
			}

			QueryResult res;
			res.data = timestampDatas.data();
			res.size = currTimestampIndex;
			return res;
		}

	private:
		std::vector<uint64_t> queryResults;
		std::vector<QueryResult::TimestampData> timestampDatas;
		vk::UniqueQueryPool queryPool;
		uint32_t currTimestampIndex;
		float timestampPeriod;
		uint64_t timestampMask;
	};
}
