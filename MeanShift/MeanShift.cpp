#include "stdafx.h"
#include <iostream>
#include <list>
#include <math.h>
#include <ctype.h> //usado para lidar com caracteres

#include <opencv2/core/utility.hpp>
#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp" 
#include <opencv2/video/background_segm.hpp>

using namespace cv;
using namespace std;

Mat image;

bool backprojection = false;
bool selectObject = false;
int trackObject = 0;
Point origin;
Rect selection;
Point2f location;

list<Point2f> points;

static void onMouse(int event, int x, int y, int, void*)
{
	if (selectObject)
	{
		selection.x = MIN(x, origin.x);
		selection.y = MIN(y, origin.y);
		selection.width = std::abs(x - origin.x);
		selection.height = std::abs(y - origin.y);

		selection &= Rect(0, 0, image.cols, image.rows);
	}

	switch (event)
	{
	case EVENT_LBUTTONDOWN:
		origin = Point(x, y);
		selection = Rect(x, y, 0, 0);
		selectObject = true;
		break;
	case EVENT_LBUTTONUP:
		selectObject = false;
		if (selection.width > 0 && selection.height > 0)
			trackObject = -1;
		break;
	}
}

static void help()
{
	system("cls");
	cout << "ESC: Fechar o programa" << endl;
	cout << "B: BackProjection" << endl;
	cout << "C: Parar rastreamento" << endl;
	cout << "P: Pausar imagem" << endl;
	cout << "L: Listar pontos" << endl;
}

static void showPoints()
{
	system("cls");
	for (auto p : points)
	{
		cout << p << " ";
	}
}

static float euclidiana(Point2f ponto1, Point2f ponto2)
{
	return sqrt(pow((ponto2.x - ponto1.x), 2) + pow((ponto2.y - ponto1.y), 2));
}

int main()
{
	cout << "Digite o nome do video: ";
	string nome;
	cin >> nome;
	VideoCapture video("C:\\Users\\moaci\\Desktop\\" + nome + ".mp4");

	if (!video.isOpened()) {
		cout << "Nao foi possivel abrir o video" << endl;
		system("pause");
		exit(0);
	}

	Rect trackWindow;
	int hsize = 16; //histograma
	float hranges[] = { 0, 256 };
	const float* phranges = hranges;

	Mat frame, hsv, hue, mask, hist, histimg = Mat::zeros(200, 320, CV_8UC3), backproj, imageBlur;
	bool paused = true;
	bool showPath = false;
	int iterations;

	int min = 50;  //0  -  50
	int max = 215; //25  -  215

	int mode = 0; // 0 = meanshift, 1 = camshift

	RotatedRect trackBox; //usado no camshift

	Mat maskPath(480, 640, CV_8UC4, Scalar(0, 0, 0, 0));
	//cvtColor(maskPath, maskPath, CV_BGR2BGRA);

	//Distancia percorrida
	float distance = 0;
	Point2f lastPoint;
	Point2f flag;

	//Tempo
	int fps = video.get(CV_CAP_PROP_FPS);
	int seconds = 0;
	int framecount = 1;

	//Densidade
	bool firstIt = true;

	Ptr<BackgroundSubtractor> pMOG = createBackgroundSubtractorMOG2();
	Mat fgMaskMOG;
	Mat firstFrame;
	Mat gray;
	Mat accumImage;
	Mat colorImage;
	Mat heatmap;

	int thresh = 2;
	int maxValue = 4;
	Mat th1;

	int speed = 10;

	//namedWindow("Histogram", 0);
	namedWindow("Video", 0);
	namedWindow("Path", 0);
	namedWindow("Heatmap1", 0);
	//namedWindow("Heatmap2", 0);

	setMouseCallback("Video", onMouse, 0);

	createTrackbar("Shift mode", "Video", &mode, 1, 0);
	//createTrackbar("min", "Video", &min, 255, 0);
	//createTrackbar("max", "Video", &max, 255, 0);

	//Para heatmap
	//createTrackbar("heatTresh", "Video", &thresh, 255, 0);
	//createTrackbar("heatMax", "Video", &maxValue, 8, 0);

	createTrackbar("Speed", "Video", &speed, 10, 0);

	help();

	video >> frame;

	for (;;)
	{
		if (!paused)
		{
			video >> frame;
			if (frame.empty())
				break;
		}

		frame.copyTo(image);
		resize(image, image, Size(640, 480), 0, 0, INTER_CUBIC);

		if (!paused)
		{
			//GaussianBlur(image, imageBlur, Size(5, 5), 0, 0);
			blur(image, imageBlur, Size(7, 7));

			cvtColor(imageBlur, hsv, COLOR_BGR2HSV);

			if (trackObject)
			{

				inRange(hsv, Scalar(0, min, MIN(min, max)),  // 70 e 215 
					Scalar(180, 256, MAX(min, max)), mask);
				int ch[] = { 0, 0 };
				hue.create(hsv.size(), hsv.depth());
				mixChannels(&hsv, 1, &hue, 1, ch, 1);

				if (trackObject < 0)
				{
					Mat roi(hue, selection), maskroi(mask, selection);
					calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
					normalize(hist, hist, 0, 255, NORM_MINMAX);

					trackWindow = selection;
					trackObject = 1;

					histimg = Scalar::all(0);
					int binW = histimg.cols / hsize;
					Mat buf(1, hsize, CV_8UC3);
					for (int i = 0; i < hsize; i++)
						buf.at<Vec3b>(i) = Vec3b(saturate_cast<uchar>(i*180. / hsize), 255, 255);
					cvtColor(buf, buf, COLOR_HSV2BGR);

					for (int i = 0; i < hsize; i++)
					{
						int val = saturate_cast<int>(hist.at<float>(i)*histimg.rows / 255);
						rectangle(histimg, Point(i*binW, histimg.rows),
							Point((i + 1)*binW, histimg.rows - val),
							Scalar(buf.at<Vec3b>(i)), -1, 8);
					}
				}

				calcBackProject(&hue, 1, 0, hist, backproj, &phranges);

				backproj &= mask;

				/*	//Mat bw = threshval < 128 ? (img < threshval) : (img > threshval);
					Mat labelImage(backproj.size(), CV_32S);
					int nLabels = connectedComponents(backproj, labelImage, 8);

					vector<int> contLabels(nLabels, 0);

					for (int r = 0; r < backproj.rows; r++) {
						for (int c = 0; c < backproj.cols; c++) {
							contLabels[labelImage.at<int>(r, c)] ++;
						}
					}

					int t1 = 50, t2 = 500;

					for (int r = 0; r < backproj.rows; r++) {
						for (int c = 0; c < backproj.cols; c++) {
							if ((contLabels[labelImage.at<int>(r, c)] < t1) || (contLabels[labelImage.at<int>(r, c)] > t2))
							{
					//			backproj.at<double>(r, c) = 0;
							}
						}
					}*/

				if (mode)
					trackBox = CamShift(backproj, trackWindow,
						TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 10, 1));
				else
					meanShift(backproj, trackWindow,
						TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 10, 1));

				if (trackWindow.area() <= 1)
				{
					int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5) / 6;
					trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
						trackWindow.x + r, trackWindow.y + r) &
						Rect(0, 0, cols, rows);
				}

				if (backprojection)
					cvtColor(backproj, image, COLOR_GRAY2BGR);

				if (mode)
				{
					ellipse(image, trackBox, Scalar(0, 0, 255), 3, LINE_AA);
					if (trackBox.center != location)
					{
						if (((trackBox.center.x - location.x > 3) || (trackBox.center.x - location.x < -3))
							&& ((trackBox.center.y - location.y > 3) || (trackBox.center.y - location.y < -3)))
						{
							lastPoint = location;
							location = trackBox.center;
							points.push_back(location);
							circle(maskPath, location, 1, CV_RGB(255, 0, 0), 4);
							if (points.size() >= 2)
							{
								distance += euclidiana(lastPoint, location);
							}
						}
					}
				}
				else
				{
					rectangle(image, trackWindow, Scalar(0, 0, 255), 3, LINE_AA);

					Point2f recCenter = (trackWindow.br() + trackWindow.tl()) * 0.5;
					if (recCenter != location)
					{
						if (((recCenter.x - location.x > 3) || (recCenter.x - location.x < -3))
							&& ((recCenter.y - location.y > 3) || (recCenter.y - location.y < -3)))
						{
							lastPoint = location;
							location = recCenter;
							points.push_back(location);
							circle(maskPath, location, 1, CV_RGB(255, 0, 0), 4);
							if (points.size() >= 2)
							{
								distance += euclidiana(lastPoint, location);
							}
						}
					}
				}

				framecount++;
				if (framecount == fps)
				{
					//if (location != flag)
					//{
					//flag = location;
					seconds++;
					//}
					framecount = 0;
				}

				if (firstIt)
				{
					firstIt = false; //N ESQUECER DE BOTAR TRUE QND ZERAR EXECUÇÂO
					image.copyTo(firstFrame);
					cvtColor(image, gray, COLOR_BGR2GRAY);
					accumImage = Mat::zeros(gray.rows, gray.cols, CV_8UC1);
				}
				else
				{
					cvtColor(imageBlur, gray, COLOR_BGR2GRAY);
					pMOG->apply(gray, fgMaskMOG);
					threshold(fgMaskMOG, th1, thresh, maxValue, THRESH_OTSU);
					add(th1, accumImage, accumImage);
				}

				applyColorMap(accumImage, colorImage, COLORMAP_HOT);
				imshow("Heatmap1", colorImage);

				if (showPath)
				{
					for (auto p : points)
					{
						circle(image, p, 1, CV_RGB(255, 0, 0), 4);
					}
				}

			}
		}
		else if (trackObject < 0)
			paused = false;

		if (selectObject && selection.width > 0 && selection.height > 0)
		{
			Mat roi(image, selection);
			bitwise_not(roi, roi);
		}

		//imshow("Histogram", histimg);
		imshow("Path", maskPath);
		imshow("Video", image);

		char c = (char)waitKey(101 - speed * 10);
		if (c == 27) //ESC
			break;
		switch (c)
		{
		case 'b':
			backprojection = !backprojection;
			break;
		case 'c':
			trackObject = 0;
			histimg = Scalar::all(0); //seta tudo os pixel pra 0
			maskPath = Scalar::all(0);
			paused = true;
			break;
		case 'p':
			paused = !paused;
			break;
		case 'l':
			showPoints();
			showPath = !showPath;
			break;
		case 'h':
			help();
			break;
		default:
			;
		}
	}

	addWeighted(colorImage, 0.7, firstFrame, 0.7, 0, heatmap);
	
	int i = 255;
	Point2f lastP = points.front();
	for (auto p : points)
	{
		circle(image, p, 1, CV_RGB(i, 0, 255 - i), 3);
		line(image, lastP, p, CV_RGB(i, 0, 255 - i), 1);
		lastP = p;
		i -= 2;
		if (i <= 0)
			i = 0;
	}
	imshow("Video", image);

	Mat img(480, 640, CV_8U, Scalar(255));

	while (points.size() > 0)
	{
		Point2f p = points.front();
		points.pop_front();
		int x = p.x;
		int y = p.y;
		//img.at<uchar>(y, x) = 0;
		circle(img, p, 13, CV_RGB(0, 0, 0), CV_FILLED, 1);
	}

	Mat draw;
	distanceTransform(img, draw, CV_DIST_L2, 5);

	// back from float to uchar, needs a heuristic scale factor here (10 works nice for the demo)
	draw.convertTo(draw, CV_8U, 10);

	applyColorMap(draw, draw, COLORMAP_HOT);

	//addWeighted(image, 0.7, draw, 0.7, 0, draw);

	//imshow("Heatmap2", draw);

	imwrite("path.jpg", maskPath);
	imwrite("path2.jpg", image);
	imwrite("heatmap1.jpg", colorImage);
	imwrite("heatmap2.jpg", heatmap);
	imwrite("heatmap3.jpg", draw);

	cout << "Video terminado" << endl;

	// 190 -- 530  -> 340 pixels = 1700mm
	//					1 pixel  = X mm
	//                       X = 5 mm
	distance = distance * 5;

	cout << endl;
	cout << "Distancia: " << distance << "mm" << endl;
	cout << "Segundos: " << seconds << "s" << endl;

	cout << "VM: " << distance / seconds << "mm/s" << endl;


	/*Mat soma(480, 640, CV_8U, Scalar(0));

	while (points.size() > 0)
	{
	Mat img(480, 640, CV_8U, Scalar(0));
		Point2f p = points.front();
		points.pop_front();
		int x = p.x;
		int y = p.y;
		//img.at<uchar>(y, x) = 0;
		circle(img, p, 13, CV_RGB(255, 255, 255), CV_FILLED, 1);
		add(img, soma, soma, noArray());
	}



	applyColorMap(soma, soma, COLORMAP_RAINBOW);

	//addWeighted(image, 0.7, draw, 0.7, 0, draw);

	imshow("teste", soma);*/


	waitKey(0);
	system("pause");

	return 0;
}

