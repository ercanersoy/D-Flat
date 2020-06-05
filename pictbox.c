/* -------------- pictbox.c -------------- */

#include "dflat.h"

typedef struct    {
    enum VectTypes vt;
    RECT rc;
} VECT;

unsigned char CharInWnd[] = "ĳڿ���ô��";

unsigned char VectCvt[3][11][2][4] = {
    {   /* --- first character in collision vector --- */
        /* ( drawing � ) ( drawing � ) */
             {{"���"},     {"���"}},
             {{"�¿"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}}    },
    {   /* --- middle character in collision vector --- */
        /* ( drawing � ) ( drawing � ) */
             {{"���"},     {"���"}},
             {{"�Ŵ"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"�Ŵ"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}}    },
    {   /* --- last character in collision vector --- */
        /* ( drawing � ) ( drawing � ) */
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"�¿"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}},
             {{"�Ŵ"},     {"���"}},
             {{"���"},     {"���"}},
             {{"���"},     {"���"}}    }
};

/* -- compute whether character is first, middle, or last -- */
static int FindVector(WINDOW wnd, RECT rc, int x, int y)
{
    RECT rcc;
    VECT *vc = wnd->VectorList;
    int i, coll = -1;
    for (i = 0; i < wnd->VectorCount; i++)    {
        if ((vc+i)->vt == VECTOR)    {
            rcc = (vc+i)->rc;
            /* --- skip the colliding vector --- */
            if (rcc.lf == rc.lf && rcc.rt == rc.rt &&
                    rcc.tp == rc.tp && rc.bt == rcc.bt)
                continue;
            if (rcc.tp == rcc.bt)    {
                /* ---- horizontal vector,
                    see if character is in it --- */
                if (rc.lf+x >= rcc.lf && rc.lf+x <= rcc.rt &&
                        rc.tp+y == rcc.tp)    {
                    /* --- it is --- */
                    if (rc.lf+x == rcc.lf)
                        coll = 0;
                    else if (rc.lf+x == rcc.rt)
                        coll = 2;
                    else 
                        coll = 1;
                }
            }
            else     {
                /* ---- vertical vector,
                    see if character is in it --- */
                if (rc.tp+y >= rcc.tp && rc.tp+y <= rcc.bt &&
                        rc.lf+x == rcc.lf)    {
                    /* --- it is --- */
                    if (rc.tp+y == rcc.tp)
                        coll = 0;
                    else if (rc.tp+y == rcc.bt)
                        coll = 2;
                    else 
                        coll = 1;
                }
            }
        }
    }
    return coll;
}

static void PaintVector(WINDOW wnd, RECT rc)
{
	int i, xi, yi, len;
    unsigned int ch, nc;
    unsigned int newch;
    static int cw, fml, vertvect, coll;

    if (rc.rt == rc.lf)    {
        /* ------ vertical vector ------- */
        nc = '�';
        vertvect = 1;
        len = rc.bt-rc.tp+1;
    }
    else     {
        /* ------ horizontal vector ------- */
        nc = '�';
        vertvect = 0;
        len = rc.rt-rc.lf+1;
    }

    for (i = 0; i < len; i++)    {
        newch = nc;
        xi = yi = 0;
        if (vertvect)
            yi = i;
        else
            xi = i;
        ch = videochar(GetClientLeft(wnd)+rc.lf+xi,
                    GetClientTop(wnd)+rc.tp+yi);
        for (cw = 0; cw < sizeof(CharInWnd); cw++)    {
            if (ch == CharInWnd[cw])    {
                /* ---- hit another vector character ---- */
                if ((coll=FindVector(wnd, rc, xi, yi)) != -1) {
                    /* compute first/middle/last subscript */
                    if (i == len-1)
                        fml = 2;
                    else if (i == 0)
                        fml = 0;
                    else
                        fml = 1;
                    newch = VectCvt[coll][cw][vertvect][fml];
                }
            }
        }
		PutWindowChar(wnd, newch, rc.lf+xi, rc.tp+yi);
    }
}

static void PaintBar(WINDOW wnd, RECT rc, enum VectTypes vt)
{
    int i, vertbar, len;
    unsigned int tys[] = {219, 178, 177, 176};
/*    unsigned int tys[] = {'�', '�', '�', '�'};
*/
    unsigned int nc = tys[vt-1];

    if (rc.rt == rc.lf)    {
        /* ------ vertical bar ------- */
        vertbar = 1;
        len = rc.bt-rc.tp+1;
    }
    else     {
        /* ------ horizontal bar ------- */
        vertbar = 0;
        len = rc.rt-rc.lf+1;
    }

    for (i = 0; i < len; i++)    {
        int xi = 0, yi = 0;
        if (vertbar)
            yi = i;
        else
            xi = i;
        PutWindowChar(wnd, nc, rc.lf+xi, rc.tp+yi);
    }
}

static void PaintMsg(WINDOW wnd)
{
    int i;
    VECT *vc = wnd->VectorList;
    for (i = 0; i < wnd->VectorCount; i++)    {
        if (vc->vt == VECTOR)
            PaintVector(wnd, vc->rc);
        else
            PaintBar(wnd, vc->rc, vc->vt);
        vc++;
    }
}

static void DrawVectorMsg(WINDOW wnd,PARAM p1,enum VectTypes vt)
{
    if (p1)    {
        VECT vc;
        wnd->VectorList = DFrealloc(wnd->VectorList,
                sizeof(VECT) * (wnd->VectorCount + 1));
        vc.vt = vt;
        vc.rc = *(RECT *)p1;
        *(((VECT *)(wnd->VectorList))+wnd->VectorCount)=vc;
        wnd->VectorCount++;
    }
}

static void DrawBoxMsg(WINDOW wnd, PARAM p1)
{
    if (p1)    {
        RECT rc = *(RECT *)p1;
        rc.bt = rc.tp;
        SendMessage(wnd, DRAWVECTOR, (PARAM) &rc, TRUE);
        rc = *(RECT *)p1;
        rc.lf = rc.rt;
        SendMessage(wnd, DRAWVECTOR, (PARAM) &rc, FALSE);
        rc = *(RECT *)p1;
        rc.tp = rc.bt;
        SendMessage(wnd, DRAWVECTOR, (PARAM) &rc, TRUE);
        rc = *(RECT *)p1;
        rc.rt = rc.lf;
        SendMessage(wnd, DRAWVECTOR, (PARAM) &rc, FALSE);
    }
}

int PictureProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    switch (msg)    {
        case PAINT:
            BaseWndProc(PICTUREBOX, wnd, msg, p1, p2);
            PaintMsg(wnd);
            return TRUE;
        case DRAWVECTOR:
            DrawVectorMsg(wnd, p1, VECTOR);
            return TRUE;
        case DRAWBOX:
            DrawBoxMsg(wnd, p1);
            return TRUE;
        case DRAWBAR:
            DrawVectorMsg(wnd, p1, (enum VectTypes)p2);
            return TRUE;
        case CLOSE_WINDOW:
            if (wnd->VectorList != NULL)
                free(wnd->VectorList);
            break;
        default:
            break;
    }
    return BaseWndProc(PICTUREBOX, wnd, msg, p1, p2);
}

static RECT PictureRect(int x, int y, int len, int hv)
{
    RECT rc;
    rc.lf = rc.rt = x;
    rc.tp = rc.bt = y;
    if (hv)
        /* ---- horizontal vector ---- */
        rc.rt += len-1;
    else
        /* ---- vertical vector ---- */
        rc.bt += len-1;
    return rc;
}

void DrawVector(WINDOW wnd, int x, int y, int len, int hv)
{
    RECT rc = PictureRect(x,y,len,hv);
    SendMessage(wnd, DRAWVECTOR, (PARAM) &rc, 0);
}

void DrawBox(WINDOW wnd, int x, int y, int ht, int wd)
{
    RECT rc;
    rc.lf = x;
    rc.tp = y;
    rc.rt = x+wd-1;
    rc.bt = y+ht-1;
    SendMessage(wnd, DRAWBOX, (PARAM) &rc, 0);
}

void DrawBar(WINDOW wnd,enum VectTypes vt,
                        int x,int y,int len,int hv)
{
    RECT rc = PictureRect(x,y,len,hv);
    SendMessage(wnd, DRAWBAR, (PARAM) &rc, (PARAM) vt);
}
