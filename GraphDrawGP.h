#pragma once

//// GDI+��
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")
//using namespace Gdiplus;

#include "..\MyLib\MyStack.h"
#include "..\MyLib\StdString.h"

enum eGraphDrawItemType
{
	EGDIT_IMG_DEF ,
	EGDIT_TEXT_DEF ,
	EGDIT_GRID ,
	EGDIT_SCALE_LINE,
	EGDIT_SCALE_TEXT,
	EGDIT_MULTI_LINE,
	EGDIT_UNKNOW
};

enum eDirection
{
	EDERCT_UP ,
	EDERCT_DOWN ,
	EDERCT_LEFT ,
	EDERCT_RIGHT 
};

class CGraphDrawItem
{
public:
	virtual void OnDraw( Gdiplus::Graphics& graphics ) = 0;
	virtual void Release() = 0;
	virtual eGraphDrawItemType GetType() = 0;
};

class CGraphMainDraw
{
	CBaseBlockStack<CGraphDrawItem*> m_drawList;
	Gdiplus::Bitmap m_bmBuf;
	Gdiplus::Graphics* m_grpBuf;
	Gdiplus::RectF m_rc;
	INT x;
	INT y;
	Gdiplus::Color m_bkColor;
public:
	CGraphMainDraw()
		: m_bmBuf(4 , 4)
	{
		m_grpBuf = NULL;
	}
	CGraphMainDraw( INT X , INT Y , INT w , INT h , Gdiplus::Color bkColor = Gdiplus::Color(0,0,0,0) )
		: m_bmBuf(w , h) 
	{
		x = X;
		y = Y;
		m_bkColor = bkColor;
		m_rc.X = (Gdiplus::REAL)X;
		m_rc.Y = (Gdiplus::REAL)Y;
		m_rc.Width = (Gdiplus::REAL)w;
		m_rc.Height = (Gdiplus::REAL)h;
		m_grpBuf = Gdiplus::Graphics::FromImage(&m_bmBuf);
	}
	~CGraphMainDraw()
	{
		CGraphDrawItem** ppGDI;
		while( (ppGDI=m_drawList.Pop()) != NULL )
			(*ppGDI)->Release();
		if(m_grpBuf)
			delete m_grpBuf;
	}
	void MoveList(CGraphMainDraw &pd)
	{
		CGraphDrawItem** ppGDI;
		sBaseBlockStackPosition pos;
		for( pd.m_drawList.GetHeadPosition( pos ) ; (ppGDI=pd.m_drawList.GetNext(pos)) != NULL ; )
		{
			m_drawList.Push(*ppGDI);
		}
		pd.m_drawList.PopToBottom();
	}
	void UpdataBuf()
	{
		CGraphDrawItem** ppGDI;
		sBaseBlockStackPosition pos;
		if(m_grpBuf)
		{
			if( m_bkColor.GetAlpha() != 0 )
			{
				Gdiplus::SolidBrush brush(m_bkColor);
				m_grpBuf->FillRectangle(&brush , m_rc);
			}
			for( m_drawList.GetHeadPosition( pos ) ; (ppGDI=m_drawList.GetNext(pos)) != NULL ; )
			{
				(*ppGDI)->OnDraw(*m_grpBuf);
			}
		}
	}
	void OnDrawInBuf( Gdiplus::Graphics& graphics )
	{
		if(m_grpBuf)
		{
			Gdiplus::Rect rc(x, y,m_bmBuf.GetWidth(), m_bmBuf.GetHeight());
			graphics.DrawImage( &m_bmBuf , rc , 0, 0, 
				(INT)m_bmBuf.GetWidth(), (INT)m_bmBuf.GetHeight() , Gdiplus::UnitPixel );
		}
	}
	void OnDrawInBuf( HDC hdc )
	{
		Gdiplus::Graphics graphics(hdc);
		OnDrawInBuf( graphics );
	}
	void OnDraw( Gdiplus::Graphics& graphics )
	{
		CGraphDrawItem** ppGDI;
		sBaseBlockStackPosition pos;
		if(m_grpBuf)
		{
			if( m_bkColor.GetAlpha() != 0 )
			{
				Gdiplus::SolidBrush brush(m_bkColor);
				m_grpBuf->FillRectangle(&brush , m_rc);
			}
			//TRACE("CGraphMainDraw\r\n");
			for( m_drawList.GetHeadPosition( pos ) ; (ppGDI=m_drawList.GetNext(pos)) != NULL ; )
			{
				(*ppGDI)->OnDraw(*m_grpBuf);
			}
			Gdiplus::Rect rc(x, y,m_bmBuf.GetWidth(), m_bmBuf.GetHeight());
			graphics.DrawImage( &m_bmBuf , rc , 0, 0, 
				(INT)m_bmBuf.GetWidth(), (INT)m_bmBuf.GetHeight() , Gdiplus::UnitPixel );
		}
		else
		{
			if( m_bkColor.GetAlpha() != 0 )
			{
				Gdiplus::SolidBrush brush(m_bkColor);
				graphics.FillRectangle(&brush , m_rc);
			}
			for( m_drawList.GetHeadPosition( pos ) ; (ppGDI=m_drawList.GetNext(pos)) != NULL ; )
			{
				(*ppGDI)->OnDraw(graphics);
			}
		}
	}
	void OnDraw( HDC hdc )
	{
		Gdiplus::Graphics graphics(hdc);
		OnDraw( graphics );
	}
	void AddItem(CGraphDrawItem* gdi)
	{
		m_drawList.Push(gdi);
	}
	void RemoveItem(CGraphDrawItem* gdi)
	{
		UINT ui;
		if( m_drawList.LookUp( gdi , &ui ) )
		{
			gdi->Release();
			m_drawList.RemoveItem( ui );
		}
	}
	void RemoveAllItem()
	{
		CGraphDrawItem** ppGDI;
		while( (ppGDI=m_drawList.Pop()) != NULL )
			(*ppGDI)->Release();
	}
	// �޻��������ܸ���
	void SetPos( INT X , INT Y )
	{
		x = X;
		y = Y;
	}
	// �޻��������ܸ���
	void Move( INT ox , INT oy )
	{
		x += ox;
		y += oy;
	}
};


class CImgDraw : public CGraphDrawItem
{
	IStream* m_ist;
	Gdiplus::Rect rc;
	INT srcx;
	INT srcy;
	INT srcwidth;
	INT srcheight;
public:
	Gdiplus::Image m_img;
	CImgDraw(IStream* ist) : m_img(ist)
	{
		m_ist = ist;
		//Ĭ�ϲ��ü���������
		rc.X = 0;
		rc.Y = 0;
		rc.Width = m_img.GetWidth();
		rc.Height = m_img.GetHeight();
		srcx = 0;
		srcy = 0;
		srcwidth = m_img.GetWidth();
		srcheight = m_img.GetHeight();
		//srcwidth = 0;
		//srcheight = 0;
	}
	~CImgDraw()
	{
		if( m_ist != NULL )
			m_ist->Release();
	}
	virtual void OnDraw( Gdiplus::Graphics& graphics )
	{
		//Gdiplus::Rect rc(x, y,srcwidth, srcheight)
		//	graphics.DrawImage( &m_img , rc);
		//Gdiplus::Rect rc(x, y,m_img.GetWidth(), m_img.GetHeight());
		graphics.DrawImage( &m_img , rc , srcx , srcy , srcwidth , srcheight , Gdiplus::UnitPixel );

		//����ĺ����ᰴͼ������ʾ�ֱ�������
		//graphics.DrawImage( &m_img , x, y);
		//graphics.DrawImage( &m_img , x, y, srcx, srcy, srcwidth, srcheight,UnitPixel);
	}
	virtual void Release()
	{
		delete this;
	}
	virtual eGraphDrawItemType GetType()
	{
		return EGDIT_IMG_DEF;
	}
	static CImgDraw* Add(IStream* ist)
	{
		return new CImgDraw(ist);
	}
	void SetPoint(INT x , INT y)
	{
		rc.X = x;
		rc.Y = y;
		rc.Width = m_img.GetWidth();
		rc.Height = m_img.GetHeight();
	}
	void SetPoint(POINT &pt)
	{
		SetPoint(pt.x, pt.y);
	}
	void GetPoint(INT& x , INT& y)
	{
		x = rc.X;
		y = rc.Y;
	}
	void GetPoint(POINT& pt)
	{
		GetPoint((INT&)pt.x, (INT&)pt.y);
	}
	void SetRect(INT x , INT y , INT width , INT height)
	{
		rc.X = x;
		rc.Y = y;
		rc.Width = width;
		rc.Height = height;
	}
	void SetRect(RECT r)
	{
		SetRect(r.left , r.top , r.right-r.left , r.bottom-r.top);
	}
	void GetRect(INT& x , INT& y , INT& width , INT& height)
	{
		x = rc.X;
		y = rc.Y;
		width = rc.Width;
		height = rc.Height;
	}
	void GetRect(RECT& r)
	{
		GetRect((INT&)r.left , (INT&)r.top , (INT&)r.right , (INT&)r.bottom);
		r.right += r.left;
		r.bottom += r.top;
	}

	void SetSrcRect(INT x , INT y , INT width , INT height)
	{
		srcx = x;
		srcy = y;
		srcwidth = width;
		srcheight = height;
	}
	void SetSrcRect(RECT r)
	{
		SetSrcRect(r.left , r.top , r.right-r.left , r.bottom-r.top);
	}
	void GetSrcRect(INT& x , INT& y , INT& width , INT& height)
	{
		x = srcx;
		y = srcy;
		width = srcwidth;
		height = srcheight;
	}
	void GetSrcRect(RECT& r)
	{
		GetSrcRect((INT&)r.left , (INT&)r.top , (INT&)r.right , (INT&)r.bottom);
		r.right += r.left;
		r.bottom += r.top;
	}

	Gdiplus::Image* GetImg()
	{
		return &m_img;
	}
};


class CTextDraw : public CGraphDrawItem
{
	std::wstring string;
	INT length;
	Gdiplus::Font font;
	Gdiplus::StringFormat stringFormat;
	Gdiplus::Color color;
	Gdiplus::RectF layoutRect;
public:
	CTextDraw(WCHAR *str ,Gdiplus::RectF lr , Gdiplus::Color col , INT fontHeight , WCHAR *fontName , Gdiplus::FontStyle style )
		: font( fontName , (Gdiplus::REAL)fontHeight , style , Gdiplus::UnitPixel )
	{
		//FontFamily fontFamily(fontName);
		color = col;
		string = str;
		length = -1;
		layoutRect = lr;
	}
	~CTextDraw()
	{
	}
	virtual void OnDraw( Gdiplus::Graphics& graphics )
	{
        Gdiplus::SolidBrush brush(color);  
		graphics.DrawString( string.c_str(), length, &font, layoutRect, &stringFormat, &brush);
	}
	virtual void Release()
	{
		delete this;
	}
	virtual eGraphDrawItemType GetType()
	{
		return EGDIT_TEXT_DEF;
	}
	// str���������ڴ棬���ݱ������ⲿ����ʹ���ַ�������
	static CTextDraw* Add(WCHAR *str ,Gdiplus::RectF lr , Gdiplus::Color col = Gdiplus::Color(255, 0, 0, 0) , INT fontHeight = 16 , 
		WCHAR *fontName = L"����" , Gdiplus::FontStyle style = Gdiplus::FontStyleRegular )
	{
		return new CTextDraw(str ,lr ,col ,fontHeight ,fontName ,style );
	}
	static CTextDraw* Add(WCHAR *str ,RECT lr , Gdiplus::Color col = Gdiplus::Color(255, 0, 0, 0) , INT fontHeight = 16 , 
		WCHAR *fontName = L"����" , Gdiplus::FontStyle style = Gdiplus::FontStyleRegular )
	{
		return new CTextDraw(str ,Gdiplus::RectF((Gdiplus::REAL)lr.left ,(Gdiplus::REAL)lr.top ,
			(Gdiplus::REAL)lr.right-lr.left , (Gdiplus::REAL)lr.bottom-lr.top) ,
			col ,fontHeight ,fontName ,style );
	}
	void SetAlignment(Gdiplus::StringAlignment align)
	{
		stringFormat.SetAlignment(align);
	}
	void SetLineAlignment(Gdiplus::StringAlignment align)
	{
		stringFormat.SetLineAlignment(align);
	}
	void SetString(WCHAR *str , INT len = -1)
	{
		string = str;
		length = len;
	}
	WCHAR* GetString()
	{
		return (WCHAR*)&string[0];
	}
	void SetRect(Gdiplus::RectF rect)
	{
		layoutRect = rect;
	}
	void SetRect(INT X , INT Y , INT width , INT height)
	{
		layoutRect.X = (Gdiplus::REAL)X;
		layoutRect.Y = (Gdiplus::REAL)Y;
		layoutRect.Width = (Gdiplus::REAL)width;
		layoutRect.Height = (Gdiplus::REAL)height;
	}
	void SetRect(RECT r)
	{
		SetRect(r.left , r.top , r.right-r.left , r.bottom-r.top);
	}
	void GetRect(INT& X , INT& Y , INT& width , INT& height)
	{
		X = (INT)layoutRect.X;
		Y = (INT)layoutRect.Y;
		width = (INT)layoutRect.Width;
		height = (INT)layoutRect.Height;
	}
	void GetRect(RECT& r)
	{
		GetRect((INT&)r.left , (INT&)r.top , (INT&)r.right , (INT&)r.bottom);
		r.right += r.left;
		r.bottom += r.top;
	}
	void GetRect(Gdiplus::RectF& r)
	{
		r = layoutRect;
	}
	void SetColor(Gdiplus::Color col)
	{
		color = col;
	}
	Gdiplus::Color GetColor()
	{
		return color;
	}
	Gdiplus::Status MeasureString(Gdiplus::Graphics& graphics ,Gdiplus::PointF &origin, Gdiplus::RectF *boundingBox )
	{
		return graphics.MeasureString( string.c_str() , string.length() , &font , origin , boundingBox );
	}
};

class CGridDraw : public CGraphDrawItem
{
	RECT rc; // �������

	Gdiplus::Color bc; // ������ɫ��͸������0Ϊ�����

	Gdiplus::Color hmc; // ˮƽ����������ɫ��͸������0Ϊ�����
	INT hmw; // ˮƽ�������߿��
	INT hmiv; // ˮƽ�������߼�� Interval

	Gdiplus::Color hsc; // ˮƽ����������ɫ��͸������0Ϊ�����
	INT hsw; // ˮƽ�������߿��
	INT hsiv; // ˮƽ�������߼�� Interval

	Gdiplus::Color vmc; // ��ֱ����������ɫ��͸������0Ϊ�����
	INT vmw; // ��ֱ�������߿��
	INT vmiv; // ��ֱ�������߼�� Interval

	Gdiplus::Color vsc; // ��ֱ����������ɫ��͸������0Ϊ�����
	INT vsw; // ��ֱ�������߿��
	INT vsiv; // ��ֱ�������߼�� Interval

	Gdiplus::Color hzc; // ˮƽ������ɫ��͸������0Ϊ�����
	INT hzw; // ˮƽ���߿��

	Gdiplus::Color vzc; // ��ֱ������ɫ��͸������0Ϊ�����
	INT vzw; // ��ֱ���߿��

	Gdiplus::Color vcc; // �Զ��崹ֱ����ɫ��͸������0Ϊ�����
	INT vcw; // �Զ��崹ֱ�߿��
	// ����λ���������������
	INT vcp; // �Զ��崹ֱ��λ��

	INT hZero; // ˮƽ����λ�� yֵ 
	INT vZero; // ��ֱ����λ�� xֵ

public:
	CGridDraw()
	{
	}
	~CGridDraw()
	{
	}
	virtual void OnDraw( Gdiplus::Graphics& graphics )
	{
		INT pos;
		Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 0), 1);
		// ������
		if( bc.GetAlpha() != 0 )
		{
			Gdiplus::SolidBrush brs(bc);
			graphics.FillRectangle(&brs, rc.left, rc.top, rc.right-rc.left , rc.bottom-rc.top);
		}
		// ��ֱ������
		if( vsc.GetAlpha() != 0 )
		{
			pen.SetColor( vsc );
			pen.SetWidth( (Gdiplus::REAL)vsw );
			for(pos = vZero+rc.left ; pos >= rc.left ; pos -= vsiv )
			{
				if(pos <= rc.right)
					graphics.DrawLine( &pen , pos , rc.top , pos , rc.bottom );
			}
			for(pos = vZero+rc.left ; pos <= rc.right ; pos += vsiv )
			{
				graphics.DrawLine( &pen , pos , rc.top , pos , rc.bottom );
			}
		}
		// ˮƽ������
		if( hsc.GetAlpha() != 0 )
		{
			pen.SetColor( hsc );
			pen.SetWidth( (Gdiplus::REAL)hsw );
			//REAL dashVals[4] = {
			//	5.0f,   // dash length 5
			//	5.0f,   // space length 2
			//	5.0f,  // dash length 15
			//	5.0f};  // space length 4

			////pen.SetDashStyle(DashStyleDot);
			//pen.SetDashPattern(dashVals, 4);
			for(pos = hZero+rc.top ; pos >= rc.top ; pos -= hsiv )
			{
				if(pos <= rc.bottom)
					graphics.DrawLine( &pen , rc.left , pos , rc.right , pos );
			}
			for(pos = hZero+rc.top ; pos <= rc.bottom ; pos += hsiv )
			{
				graphics.DrawLine( &pen , rc.left , pos , rc.right , pos );
			}
			//pen.SetDashStyle(DashStyleSolid);
		}
		// ��ֱ������
		if( vmc.GetAlpha() != 0 )
		{
			pen.SetColor( vmc );
			pen.SetWidth( (Gdiplus::REAL)vmw );
			for(pos = vZero+rc.left ; pos >= rc.left ; pos -= vmiv )
			{
				if(pos <= rc.right)
					graphics.DrawLine( &pen , pos , rc.top , pos , rc.bottom );
			}
			for(pos = vZero+rc.left ; pos <= rc.right ; pos += vmiv )
			{
				graphics.DrawLine( &pen , pos , rc.top , pos , rc.bottom );
			}
		}
		// ˮƽ������
		if( hmc.GetAlpha() != 0 )
		{
			pen.SetColor( hmc );
			pen.SetWidth( (Gdiplus::REAL)hmw );
			for(pos = hZero+rc.top ; pos >= rc.top ; pos -= hmiv )
			{
				if(pos <= rc.bottom)
					graphics.DrawLine( &pen , rc.left , pos , rc.right , pos );
			}
			for(pos = hZero+rc.top ; pos <= rc.bottom ; pos += hmiv )
			{
				graphics.DrawLine( &pen , rc.left , pos , rc.right , pos );
			}
		}
		// ��ֱ����
		if( vzc.GetAlpha() != 0 && (vZero+rc.left) < rc.right )
		{
			pen.SetColor( vzc );
			pen.SetWidth( (Gdiplus::REAL)vzw );
			graphics.DrawLine( &pen , vZero+rc.left , rc.top , vZero+rc.left , rc.bottom );
		}
		// ˮƽ����
		if( hzc.GetAlpha() != 0 && (hZero+rc.top) < rc.bottom )
		{
			pen.SetColor( hzc );
			pen.SetWidth( (Gdiplus::REAL)hzw );
			graphics.DrawLine( &pen , rc.left , hZero+rc.top , rc.right , hZero+rc.top );
		}
		// �Զ��崹ֱ��
		if( vcc.GetAlpha() != 0 && (vcp+rc.left) < rc.right )
		{
			pen.SetColor( vcc );
			pen.SetWidth( (Gdiplus::REAL)vcw );
			graphics.DrawLine( &pen , vcp+rc.left , rc.top , vcp+rc.left , rc.bottom );
		}
	}
	virtual void Release()
	{
		delete this;
	}
	virtual eGraphDrawItemType GetType()
	{
		return EGDIT_GRID;
	}
	static CGridDraw* Add(RECT _rc, // �������
		Gdiplus::Color _hmc,INT _hmw,INT _hmiv,  // ˮƽ����������ɫ����ȡ����
		Gdiplus::Color _hsc,INT _hsw,INT _hsiv,  // ˮƽ����������ɫ����ȡ����
		Gdiplus::Color _vmc,INT _vmw,INT _vmiv,  // ��ֱ����������ɫ����ȡ����
		Gdiplus::Color _vsc,INT _vsw,INT _vsiv,  // ��ֱ����������ɫ����ȡ����
		Gdiplus::Color _vcc,INT _vcw,INT _vcp,  // �Զ��崹ֱ�ߡ���ȡ����
		Gdiplus::Color _hzc,INT _hzw,  // ˮƽ������ɫ�����
		Gdiplus::Color _vzc,INT _vzw,  // ��ֱ������ɫ�����
		INT _hZero , INT _vZero , Gdiplus::Color _bc ) // ˮƽ����ֱ����λ��,����ɫ
	{
		CGridDraw* pgd = new CGridDraw();
		pgd->Init(_rc,_hmc,_hmw,_hmiv,_hsc,_hsw,_hsiv,_vmc,_vmw,_vmiv,
			_vsc,_vsw,_vsiv,_vcc,_vcw,_vcp,_hzc,_hzw,_vzc,_vzw,_hZero,_vZero,_bc);
		return pgd;
	}
	void Init(RECT _rc, // �������
		Gdiplus::Color _hmc,INT _hmw,INT _hmiv,  // ˮƽ����������ɫ����ȡ����
		Gdiplus::Color _hsc,INT _hsw,INT _hsiv,  // ˮƽ����������ɫ����ȡ����
		Gdiplus::Color _vmc,INT _vmw,INT _vmiv,  // ��ֱ����������ɫ����ȡ����
		Gdiplus::Color _vsc,INT _vsw,INT _vsiv,  // ��ֱ����������ɫ����ȡ����
		Gdiplus::Color _vcc,INT _vcw,INT _vcp,  // �Զ��崹ֱ�ߡ���ȡ����
		Gdiplus::Color _hzc,INT _hzw,  // ˮƽ������ɫ�����
		Gdiplus::Color _vzc,INT _vzw,  // ��ֱ������ɫ�����
		INT _hZero , INT _vZero , Gdiplus::Color _bc ) // ˮƽ����ֱ����λ��,����ɫ
	{
		rc = _rc;
		hmc = _hmc; hmw = _hmw; hmiv = _hmiv;
		hsc = _hsc; hsw = _hsw; hsiv = _hsiv;
		vmc = _vmc; vmw = _vmw; vmiv = _vmiv;
		vsc = _vsc; vsw = _vsw; vsiv = _vsiv;
		vcc = _vcc; vcw = _vcw; vcp = _vcp;
		hzc = _hzc; hzw = _hzw;
		vzc = _vzc; vzw = _vzw; 
		hZero = _hZero; vZero = _vZero;
		bc = _bc;
	}
	void SetRect(RECT _rc)
	{
		rc = _rc;
	}
};

class CScaleLineDraw : public CGraphDrawItem
{
	RECT rc; // �������

	Gdiplus::Color mc; // ����������ɫ��͸������0Ϊ�����
	INT mw; // �������߿��
	INT ml; // �������߳���
	INT miv; // �������߼�� Interval

	INT zero; // ����λ��
	Gdiplus::StringAlignment alig; // �̶��߶��뷽ʽ StringAlignmentNear:Top��Left
	BOOL bHorizontal; // ˮƽ����

public:
	CScaleLineDraw()
	{
	}
	~CScaleLineDraw()
	{
	}
	virtual void OnDraw( Gdiplus::Graphics& graphics )
	{
		INT pos;
		INT f,l;
		Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 0), 1);
		if( bHorizontal )
		{
			// ˮƽ���̶���
			if( mc.GetAlpha() != 0 )
			{
				switch( alig )
				{
				case Gdiplus::StringAlignmentCenter:
					f = rc.top + (rc.bottom-rc.top-ml)/2;
					l = f+ml;
					break;
				case Gdiplus::StringAlignmentFar:
					l = rc.bottom;
					f = l-ml;
					break;
				case Gdiplus::StringAlignmentNear:
				default:
					f = rc.top;
					l = f+ml;
					break;
				}
				pen.SetColor( mc );
				pen.SetWidth( (Gdiplus::REAL)mw );
				for(pos = zero+rc.left ; pos >= rc.left ; pos -= miv )
				{
					if(pos <= rc.right)
						graphics.DrawLine( &pen , pos , f , pos , l );
				}
				for(pos = zero+rc.left ; pos <= rc.right ; pos += miv )
				{
					graphics.DrawLine( &pen , pos , f , pos , l );
				}
			}
		}
		else
		{
			// ��ֱ���̶���
			if( mc.GetAlpha() != 0 )
			{
				switch( alig )
				{
				case Gdiplus::StringAlignmentCenter:
					f = rc.left + (rc.right-rc.left-ml)/2;
					l = f+ml;
					break;
				case Gdiplus::StringAlignmentNear:
					f = rc.left;
					l = f+ml;
					break;
				case Gdiplus::StringAlignmentFar:
				default:
					l = rc.right;
					f = l-ml;
					break;
				}
				pen.SetColor( mc );
				pen.SetWidth( (Gdiplus::REAL)mw );
				for(pos = zero+rc.top ; pos >= rc.top ; pos -= miv )
				{
					if(pos <= rc.bottom)
						graphics.DrawLine( &pen , f , pos , l , pos );
				}
				for(pos = zero+rc.top ; pos <= rc.bottom ; pos += miv )
				{
					graphics.DrawLine( &pen , f , pos , l , pos );
				}
			}
		}
	}
	virtual void Release()
	{
		delete this;
	}
	virtual eGraphDrawItemType GetType()
	{
		return EGDIT_SCALE_LINE;
	}
	static CScaleLineDraw* Add(RECT _rc ,  // �������
		BOOL _bHorizontal , Gdiplus::StringAlignment _alig , // �Ƿ�ˮƽ����,���뷽ʽ
		Gdiplus::Color _mc,INT _mw,INT _ml,INT _miv,  // ����������ɫ����ȡ����ȡ����
		INT _zero ) // ����λ��
	{
		CScaleLineDraw* psld = new CScaleLineDraw();
		psld->Init(_rc,_bHorizontal,_alig,_mc,_mw,_ml,_miv,_zero);
		return psld;
	}
	void Init(RECT _rc ,  // �������
		BOOL _bHorizontal , Gdiplus::StringAlignment _alig , // �Ƿ�ˮƽ����,���뷽ʽ
		Gdiplus::Color _mc,INT _mw,INT _ml,INT _miv,  // ����������ɫ����ȡ����ȡ����
		INT _zero ) // ����λ��
	{
		rc = _rc;
		bHorizontal = _bHorizontal; alig = _alig; zero = _zero;
		mc = _mc; mw = _mw; ml = _ml; miv = _miv;
	}
	void SetRect(RECT _rc)
	{
		rc = _rc;
	}
};

class CScaleTextDraw : public CGraphDrawItem
{
	RECT rc; // �������

	Gdiplus::Color color; // ��ɫ
	Gdiplus::REAL tzv; // ��ֵ
	Gdiplus::REAL viv; // ���ֵ
	INT fontHeight; // �������ָ߶�
	INT fontWidth; // �������ֿ��
	INT tiv; // �������߼�� Interval

	INT zero; // ����λ��
	BOOL bHorizontal; // ˮƽ����
	std::wstring formatStr;

	Gdiplus::Font font;
	Gdiplus::StringFormat stringFormat;

public:
	CScaleTextDraw(RECT _rc , // �������
		BOOL _bHorizontal , Gdiplus::Color _color , // �Ƿ�ˮƽ�����ı���ɫ
		Gdiplus::REAL _tzv , Gdiplus::REAL _viv , INT _tiv ,  // ��ֵ����ֵ�����λ�ü��
		INT _zero , WCHAR *_formatStr , INT _fontHeight , INT _fontWidth ,// ��ֵλ�á���ʽ��������߶� 
		WCHAR *fontName , Gdiplus::FontStyle style ) // ����������������
		: font( fontName , (Gdiplus::REAL)_fontHeight , style , Gdiplus::UnitPixel )
	{
		rc = _rc;
		bHorizontal = _bHorizontal; zero = _zero;
		color = _color; tzv = _tzv; 
		tiv = _tiv;  viv =_viv;
		formatStr = _formatStr;
		fontHeight = _fontHeight;
		fontWidth = _fontWidth;
	}
	~CScaleTextDraw()
	{
	}
	virtual void OnDraw( Gdiplus::Graphics& graphics )
	{
		Gdiplus::SolidBrush brush(color);  
		INT pos;
		std::wstring str;
		Gdiplus::REAL rz;
		Gdiplus::RectF layoutRect;
		layoutRect.X = (Gdiplus::REAL)rc.left;
		layoutRect.Y = (Gdiplus::REAL)rc.top;
		layoutRect.Width = (Gdiplus::REAL)fontWidth;
		layoutRect.Height = (Gdiplus::REAL)fontHeight+5.0f; // ��֧�ֻ���
		if( bHorizontal )
		{
			for(pos = zero+rc.left , rz = tzv; pos >= rc.left ; pos -= tiv , rz += viv)
			{
				layoutRect.X = (Gdiplus::REAL)pos;
				str = StrFormat( formatStr.c_str() , rz );
				graphics.DrawString( str.c_str() , str.length() , &font, layoutRect, &stringFormat, &brush);
			}
			for(pos = zero+rc.left , rz = tzv; pos <= rc.right ; pos += tiv , rz -= viv)
			{
				layoutRect.X = (Gdiplus::REAL)pos;
				str = StrFormat( formatStr.c_str() , rz );
				graphics.DrawString( str.c_str() , str.length() , &font, layoutRect, &stringFormat, &brush);
			}
		}
		else
		{
			for(pos = zero+rc.top , rz = tzv ; pos >= rc.top ; pos -= tiv , rz += viv)
			{
				layoutRect.Y = (Gdiplus::REAL)pos;
				str = StrFormat( formatStr.c_str() , rz );
				graphics.DrawString( str.c_str() , str.length() , &font, layoutRect, &stringFormat, &brush);
			}
			for(pos = zero+rc.top , rz = tzv ; pos <= rc.bottom ; pos += tiv , rz -= viv)
			{
				layoutRect.Y = (Gdiplus::REAL)pos;
				str = StrFormat( formatStr.c_str() , rz );
				graphics.DrawString( str.c_str() , str.length() , &font, layoutRect, &stringFormat, &brush);
			}
		}
	}
	virtual void Release()
	{
		delete this;
	}
	virtual eGraphDrawItemType GetType()
	{
		return EGDIT_SCALE_TEXT;
	}
	static CScaleTextDraw* Add(RECT _rc , // �������
		BOOL _bHorizontal , Gdiplus::Color _color , // �Ƿ�ˮƽ�����ı���ɫ
		Gdiplus::REAL _tzv , Gdiplus::REAL _viv , INT _tiv ,  // ��ֵ����ֵ�����λ�ü��
		INT _zero , INT _fontWidth , WCHAR *_formatStr = L"%f" , INT _fontHeight = 16 , // ��ֵλ�á���ʽ��������߶� 
		WCHAR *fontName = L"����" , Gdiplus::FontStyle style = Gdiplus::FontStyleRegular ) // ����������������
	{
		CScaleTextDraw* pstd = new CScaleTextDraw(_rc ,_bHorizontal , _color ,
			_tzv ,_viv,_tiv ,_zero ,_formatStr ,_fontHeight ,_fontWidth , fontName , style );
		return pstd;
	}
	void SetAlignment(Gdiplus::StringAlignment align)
	{
		stringFormat.SetAlignment(align);
	}
	void Update(Gdiplus::REAL _tzv , Gdiplus::REAL _viv)
	{
		tzv = _tzv;  viv =_viv;
	}
	void SetRect(RECT _rc)
	{
		rc = _rc;
	}
};

class CMultilineDraw : public CGraphDrawItem
{
	Gdiplus::Pen pen;
	Gdiplus::PointF *points;
	Gdiplus::RectF clipRect;
	INT count;
	BOOL bShow;
public:
	CMultilineDraw()
		: pen ( Gdiplus::Color(255,0,0,0) , 1 )
	{
		points = NULL;
		bShow = TRUE;
	}
	~CMultilineDraw()
	{
	}
	virtual void OnDraw( Gdiplus::Graphics& graphics )
	{
		if( points == NULL || !bShow )
			return;
		graphics.SetClip(clipRect);
		Gdiplus::Status s = graphics.DrawLines( &pen , points , count );
		graphics.ResetClip();
		//Gdiplus::Status s = graphics.DrawBeziers( &pen , points , count );
		//static INT i = 0;
		//TRACE("CMultilineDraw::OnDraw %i  %i\r\n" , i++,s);
	}
	virtual void Release()
	{
		delete this;
	}
	virtual eGraphDrawItemType GetType()
	{
		return EGDIT_MULTI_LINE;
	}
	static CMultilineDraw* Add(RECT &rc , Gdiplus::Color color , INT width , Gdiplus::PointF *_points , INT _count)
	{
		CMultilineDraw* pmld = new CMultilineDraw();
		pmld->Init( rc , color , width , _points , _count );
		return pmld;
	}
	void Init(RECT &rc , Gdiplus::Color color , INT width , Gdiplus::PointF *_points , INT _count) 
	{
		pen.SetColor( color );
		pen.SetWidth( (Gdiplus::REAL)width );
		points = _points;
		count = _count;
		clipRect.X = (Gdiplus::REAL)rc.left;
		clipRect.Y = (Gdiplus::REAL)rc.top;
		clipRect.Width = (Gdiplus::REAL)(rc.right-rc.left);
		clipRect.Height = (Gdiplus::REAL)(rc.bottom-rc.top);
	}
	void Update( Gdiplus::PointF *_points , INT _count )
	{
		points = _points;
		count = _count;
	}
	void SetLine(Gdiplus::Color color , INT width)
	{
		pen.SetColor( color );
		pen.SetWidth( (Gdiplus::REAL)width );
	}
	void Show(BOOL _bShow)
	{
		bShow = _bShow;
	}
	void SetRect(RECT &rc)
	{
		clipRect.X = (Gdiplus::REAL)rc.left;
		clipRect.Y = (Gdiplus::REAL)rc.top;
		clipRect.Width = (Gdiplus::REAL)(rc.right-rc.left);
		clipRect.Height = (Gdiplus::REAL)(rc.bottom-rc.top);
	}
};

class CFormatTextDraw : public CGraphDrawItem
{
	std::wstring fstr;
	Gdiplus::Font font;
	Gdiplus::Font font_s;
	Gdiplus::RectF layoutRect;
	Gdiplus::StringFormat stringFormat;
	Gdiplus::Color color;
public:
	CFormatTextDraw(WCHAR *str ,Gdiplus::RectF lr , Gdiplus::Color col , INT fontHeight , WCHAR *fontName , Gdiplus::FontStyle style )
		: font( fontName , (Gdiplus::REAL)fontHeight , style , Gdiplus::UnitPixel ) , 
		font_s( fontName , (Gdiplus::REAL)(fontHeight/2) , style , Gdiplus::UnitPixel ) , 
		fstr(str) , layoutRect(lr) , color(col)
	{
	}
	~CFormatTextDraw()
	{
	}
	virtual void OnDraw( Gdiplus::Graphics& graphics )
	{
		std::wstring str1 = fstr;
		PWSTR pfbuf = (PWSTR)fstr.c_str();
		PWSTR pbuf = (PWSTR)str1.c_str();
		INT i , k  , j , len;
		Gdiplus::SolidBrush brush(color);
		Gdiplus::RectF textRc;
		textRc = layoutRect;
		Gdiplus::PointF origin;
		origin.X = 0.0f;
		origin.Y = 0.0f;
		Gdiplus::RectF boundingBox;
		for( i = 0 , len = fstr.length() , j = 0 ; i < len ; i++ )
		{
			if( pfbuf[i] == L'#' )
			{
				switch(pfbuf[i+1])
				{
				case L'u':
					for( k = i+2 ; k < (len-1) ; k++ )
						if( pfbuf[k] == L'#' && pfbuf[k+1] == L'u' )
							break;
					// �ҵ�
					if( k < (len-1) )
					{
						graphics.DrawString( pbuf, j, &font, textRc, &stringFormat, &brush);
						graphics.MeasureString( pbuf, j, &font , origin , &boundingBox );
						textRc.X += boundingBox.Width;
						textRc.Width -= boundingBox.Width;
						if( textRc.Width <= 0.0f )
							break;

						for( j = i+2 ; j < k ; j++ )
							pbuf[j-i-2] = pfbuf[j];
						pbuf[j-i-2] = L'\0';
						graphics.DrawString( pbuf, j-i-2, &font_s, textRc, &stringFormat, &brush);
						graphics.MeasureString( pbuf, j-i-2, &font_s , origin , &boundingBox );
						textRc.X += boundingBox.Width;
						textRc.Width -= boundingBox.Width;
						j = 0;
						if( textRc.Width <= 0.0f )
							break;
						i = k+1; // ���滹Ҫ��1
						continue;
					}
					break;
				default:
					break;
				}
			} // end if
			if( textRc.Width <= 0.0f )
				break;
			pbuf[j] = pfbuf[i];
			j++;
		} // end for
		if( j > 0 )
		{
			graphics.DrawString( pbuf, j, &font, textRc, &stringFormat, &brush);
		}
	}
	virtual void Release()
	{
		delete this;
	}
	virtual eGraphDrawItemType GetType()
	{
		return EGDIT_TEXT_DEF;
	}
	// str���������ڴ棬���ݱ������ⲿ����ʹ���ַ�������
	static CFormatTextDraw* Add(WCHAR *str ,Gdiplus::RectF lr , Gdiplus::Color col = Gdiplus::Color(255, 0, 0, 0) , INT fontHeight = 16 , 
		WCHAR *fontName = L"����" , Gdiplus::FontStyle style = Gdiplus::FontStyleRegular )
	{
		return new CFormatTextDraw( str ,lr ,col ,fontHeight ,fontName ,style );
	}
	static CFormatTextDraw* Add(WCHAR *str ,RECT lr , Gdiplus::Color col = Gdiplus::Color(255, 0, 0, 0) , INT fontHeight = 16 , 
		WCHAR *fontName = L"����" , Gdiplus::FontStyle style = Gdiplus::FontStyleRegular )
	{
		return new CFormatTextDraw(str ,Gdiplus::RectF((Gdiplus::REAL)lr.left ,(Gdiplus::REAL)lr.top ,
			(Gdiplus::REAL)lr.right-lr.left , (Gdiplus::REAL)lr.bottom-lr.top) ,
			col ,fontHeight ,fontName ,style );
	}
	void SetString(WCHAR *str)
	{
		fstr = str;
	}
	WCHAR* GetString()
	{
		return (WCHAR*)fstr.c_str();
	}
	void SetRect(INT X , INT Y , INT width , INT height)
	{
		layoutRect.X = (Gdiplus::REAL)X;
		layoutRect.Y = (Gdiplus::REAL)Y;
		layoutRect.Width = (Gdiplus::REAL)width;
		layoutRect.Height = (Gdiplus::REAL)height;
	}
	void SetRect(Gdiplus::RectF &rc)
	{
		layoutRect = rc;
	}
	Gdiplus::REAL GetWidth( Gdiplus::Graphics& graphics )
	{
		std::wstring str1 = fstr;
		PWSTR pfbuf = (PWSTR)fstr.c_str();
		PWSTR pbuf = (PWSTR)str1.c_str();
		INT i , k  , j , len;
		Gdiplus::RectF textRc;
		textRc.X = 0.0f;
		textRc.Y = 0.0f;
		textRc.Width = 10000.0f;
		textRc.Height = 10000.0f;
		Gdiplus::PointF origin;
		origin.X = 0.0f;
		origin.Y = 0.0f;
		Gdiplus::RectF boundingBox;
		for( i = 0 , len = fstr.length() , j = 0 ; i < len ; i++ )
		{
			if( pfbuf[i] == L'#' )
			{
				switch(pfbuf[i+1])
				{
				case L'u':
					for( k = i+2 ; k < (len-1) ; k++ )
						if( pfbuf[k] == L'#' && pfbuf[k+1] == L'u' )
							break;
					// �ҵ�
					if( k < (len-1) )
					{
						pbuf[j] = L'\0';
						graphics.MeasureString( pbuf, j, &font , origin , &boundingBox );
						textRc.X += boundingBox.Width;
						if( textRc.Width <= 0.0f )
							break;

						for( j = i+2 ; j < k ; j++ )
							pbuf[j-i-2] = pfbuf[j];
						pbuf[j-i-2] = L'\0';
						graphics.MeasureString( pbuf, j-i-2, &font_s , origin , &boundingBox );
						textRc.X += boundingBox.Width;
						j = 0;
						if( textRc.Width <= 0.0f )
							break;
						i = k+1; // ���滹Ҫ��1
						continue;
					}
					break;
				default:
					break;
				}
			} // end if
			if( textRc.Width <= 0.0f )
				break;
			pbuf[j] = pfbuf[i];
			j++;
		} // end for
		if( j > 0 )
		{
			pbuf[j] = L'\0';
			graphics.MeasureString( pbuf, j, &font , origin , &boundingBox );
			textRc.X += boundingBox.Width;
		}
		return textRc.X;
	}
	void SetColor(Gdiplus::Color col)
	{
		color = col;
	}
	Gdiplus::Color GetColor()
	{
		return color;
	}
};