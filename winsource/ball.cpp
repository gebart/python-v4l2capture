
#include <streams.h>
#include "ball.h"


//------------------------------------------------------------------------------
// Name: CBall::CBall(()
// Desc: Constructor for the ball class. The default arguments provide a
//       reasonable image and ball size.
//------------------------------------------------------------------------------
CBall::CBall(int iImageWidth, int iImageHeight, int iBallSize) :
    m_iImageWidth(iImageWidth),
    m_iImageHeight(iImageHeight),
    m_iBallSize(iBallSize),
    m_iAvailableWidth(iImageWidth - iBallSize),
    m_iAvailableHeight(iImageHeight - iBallSize),
    m_x(0),
    m_y(0),
    m_xDir(RIGHT),
    m_yDir(UP)
{
    // Check we have some (arbitrary) space to bounce in.
    ASSERT(iImageWidth > 2*iBallSize);
    ASSERT(iImageHeight > 2*iBallSize);

    // Random position for showing off a video mixer
    m_iRandX = rand();
    m_iRandY = rand();

} // (Constructor)


//------------------------------------------------------------------------------
// Name: CBall::PlotBall()
// Desc: Positions the ball on the memory buffer.
//       Assumes the image buffer is arranged as Row 1,Row 2,...,Row n
//       in memory and that the data is contiguous.
//------------------------------------------------------------------------------
void CBall::PlotBall(BYTE pFrame[], BYTE BallPixel[], int iPixelSize)
{
    ASSERT(m_x >= 0);
    ASSERT(m_x <= m_iAvailableWidth);
    ASSERT(m_y >= 0);
    ASSERT(m_y <= m_iAvailableHeight);
    ASSERT(pFrame != NULL);
    ASSERT(BallPixel != NULL);

    // The current byte of interest in the frame
    BYTE *pBack;
    pBack = pFrame;     

    // Plot the ball into the correct location
    BYTE *pBall = pFrame + ( m_y * m_iImageWidth * iPixelSize) + m_x * iPixelSize;

    for(int row = 0; row < m_iBallSize; row++)
    {
        for(int col = 0; col < m_iBallSize; col++)
        {
            // For each byte fill its value from BallPixel[]
            for(int i = 0; i < iPixelSize; i++)
            {  
                if(WithinCircle(col, row))
                {
                    *pBall = BallPixel[i];
                }
                pBall++;
            }
        }
        pBall += m_iAvailableWidth * iPixelSize;
    }

} // PlotBall


//------------------------------------------------------------------------------
// CBall::BallPosition()
// 
// Returns the 1-dimensional position of the ball at time t millisecs
//      (note that millisecs runs out after about a month!)
//------------------------------------------------------------------------------
int CBall::BallPosition(int iPixelTime, // Millisecs per pixel
                        int iLength,    // Distance between the bounce points
                        int time,       // Time in millisecs
                        int iOffset)    // For a bit of randomness
{
    // Calculate the position of an unconstrained ball (no walls)
    // then fold it back and forth to calculate the actual position

    int x = time / iPixelTime;
    x += iOffset;
    x %= 2 * iLength;

    // check it is still in bounds
    if(x > iLength)
    {    
        x = 2*iLength - x;
    }
    return x;

} // BallPosition


//------------------------------------------------------------------------------
// CBall::MoveBall()
//
// Set (m_x, m_y) to the new position of the ball.  move diagonally
// with speed m_v in each of x and y directions.
// Guarantees to keep the ball in valid areas of the frame.
// When it hits an edge the ball bounces in the traditional manner!.
// The boundaries are (0..m_iAvailableWidth, 0..m_iAvailableHeight)
//
//------------------------------------------------------------------------------
void CBall::MoveBall(CRefTime rt)
{
    m_x = BallPosition(10, m_iAvailableWidth, rt.Millisecs(), m_iRandX);
    m_y = BallPosition(10, m_iAvailableHeight, rt.Millisecs(), m_iRandY);

} // MoveBall


//------------------------------------------------------------------------------
// CBall:WithinCircle()
//
// Return TRUE if (x,y) is within a circle radius S/2, center (S/2, S/2)
//      where S is m_iBallSize else return FALSE
//------------------------------------------------------------------------------
inline BOOL CBall::WithinCircle(int x, int y)
{
    unsigned int r = m_iBallSize / 2;

    if((x-r)*(x-r) + (y-r)*(y-r)  < r*r)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

} // WithinCircle


