#pragma once

namespace happy
{
	template <typename VERTEX>
	class WidgetBuffer
	{
	public:
		WidgetBuffer(ID3D11Device *pDevice, size_t vertices)
			: m_Size(vertices)
		{
			D3D11_BUFFER_DESC vtxDesc;
			ZeroMemory(&vtxDesc, sizeof(vtxDesc));

			vector<VERTEX> data(vertices);

			vtxDesc.ByteWidth = (UINT)(vertices * sizeof(VERTEX));
			vtxDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			vtxDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vtxDesc.Usage = D3D11_USAGE_DYNAMIC;
			D3D11_SUBRESOURCE_DATA vtxData = { 0 };
			vtxData.pSysMem = data.data();

			THROW_ON_FAIL(pDevice->CreateBuffer(&vtxDesc, &vtxData, m_pVertexBuffer.GetAddressOf()));
		}

		// begin drawing widgets
		void begin(ID3D11DeviceContext *pContext) const
		{
			pContext->Map(m_pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m_Mapped);

			m_Offset = 0;
		}

		// draw widgets. must be between begin/end
		template <typename OBJECT, typename CONVERT>
		void draw(ID3D11DeviceContext *pContext, OBJECT* objects, size_t objectCount, size_t verticesPerObject, CONVERT convertFn) const
		{
			VERTEX* verticesOut = (VERTEX*)m_Mapped.pData;

			size_t objectsPerBatch = m_Size / verticesPerObject;

			size_t objectsDrawn = 0;
			while (objectsDrawn < objectCount)
			{
				size_t verticesLeftInBuffer = m_Size - m_Offset;
				size_t objectsLeftInBuffer = verticesLeftInBuffer / verticesPerObject;

				size_t objectsLeftInBatch = objectCount - objectsDrawn;

				if (objectsLeftInBatch < objectsLeftInBuffer)
				{
					convertFn(&verticesOut[m_Offset], &objects[objectsDrawn], objectsLeftInBatch);
					m_Offset += objectsLeftInBatch * verticesPerObject;
					objectsDrawn += objectsLeftInBatch;
				}
				else
				{
					convertFn(&verticesOut[m_Offset], &objects[objectsDrawn], objectsLeftInBuffer);
					m_Offset += objectsLeftInBuffer * verticesPerObject;
					objectsDrawn += objectsLeftInBuffer;

					pContext->Unmap(m_pVertexBuffer.Get(), 0);
					render(pContext);
					pContext->Map(m_pVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m_Mapped);
					verticesOut = (VERTEX*)m_Mapped.pData;

					m_Offset = 0;
				}
			}
		}

		// end drawing widgets
		void end(ID3D11DeviceContext *pContext) const
		{
			pContext->Unmap(m_pVertexBuffer.Get(), 0);

			if (m_Offset > 0)
			{
				render(pContext);
			}
		}

	private:

		void render(ID3D11DeviceContext *pContext) const
		{
			UINT strides = (UINT)sizeof(VERTEX);
			UINT offsets = 0;
			ID3D11Buffer* buffer = m_pVertexBuffer.Get();
			pContext->IASetVertexBuffers(0, 1, &buffer, &strides, &offsets);
			pContext->Draw((UINT)m_Offset, 0);
		}
		
		ComPtr<ID3D11Buffer> m_pVertexBuffer;

		mutable D3D11_MAPPED_SUBRESOURCE m_Mapped;

		mutable size_t m_Offset;

		size_t m_Size;
	};
}