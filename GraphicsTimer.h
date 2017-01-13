#pragma once

namespace happy
{
	class GraphicsTimer
	{
	public:
		GraphicsTimer(ID3D11Device& device, ID3D11DeviceContext& context);

		void setLogDump(function<void(const char *)> dumpFn);

		ID3D11Query* beginZone(const char *tag);

		void endZone(ID3D11Query* query);

		void begin();

		void end();

	private:
		friend class GfxZone;

		ComPtr<ID3D11Query>  m_QueriesDisjoint[2];

		ID3D11Device*        m_pDevice;
		ID3D11DeviceContext* m_pContext;
		ID3D11Query*         m_pCurrentDisjoint;
		ID3D11Query*         m_pRunningDisjoint;

		struct query
		{
			void reset();
			void handle(ID3D11DeviceContext *context, UINT64 frequency, BOOL disjoint);
			void report(std::ostream &os, int level);

			std::string m_Tag;

			query *m_Parent = nullptr;
			unsigned m_Counter = 0;
			vector<query> m_SubQueries;

			ComPtr<ID3D11Query> m_Q[4];

			ID3D11Query* m_pCurrentBegin;
			ID3D11Query* m_pCurrentEnd;
			ID3D11Query* m_pRunningBegin;
			ID3D11Query* m_pRunningEnd;

			vector<double> m_avg;
			int m_ptr = 0;
		};

		unsigned m_Frames = 0;

		query m_RootQuery;
		query *m_pCurrentQuery;

		function<void(const char *)> m_DumpFn;
	};
}